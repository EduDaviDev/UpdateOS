#!/usr/bin/env bash
# ============================================================
#  makeapps.sh — compila um app do UpOS para ELF32 carregável.
#
#  Uso:
#     makeapps.sh <app>
#
#  <app> pode ser:
#     - nome de uma pasta em assets/Apps/Codes/   (ex.: helloworld)
#     - um arquivo .c/.cpp/.c++/.asm/.s           (caminho ou relativo)
#
#  Se for pasta, procura por main.<ext> ou o único fonte encontrado.
#
#  Saída:
#     assets/Apps/Binaries/<nome>.elf   <- artefato final (ELF32 i386)
#     assets/Apps/Build/<nome>.o        <- objeto intermediário
#
#  O ELF produzido tem PT_LOADs prontos pro loader do kernel
#  (load_elf32), com permissões RX/RW derivadas do linker script.
#
#  Variáveis de ambiente:
#     NO_CRT0=1   não linka crt0.{S,s,asm}
#     KEEP_OBJ=0  remove o .o intermediário após o link
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"

ASSETS_DIR="$BASE_DIR/assets"
APPS_DIR="$ASSETS_DIR/Apps"
APPS_CODES_DIR="$APPS_DIR/Codes"
APPS_BUILD_DIR="$APPS_DIR/Build"
APPS_BIN_DIR="$APPS_DIR/Binaries"

mkdir -p "$APPS_BUILD_DIR" "$APPS_BIN_DIR"

# ------------------------------------------------------------
#  Localiza o linker script
# ------------------------------------------------------------
LINKER_SCRIPT=""
for candidate in \
    "$BASE_DIR/apps.linker.ld" \
    "$APPS_DIR/apps.linker.ld" \
    "$APPS_DIR/linker.ld" \
    "$SCRIPT_DIR/apps.linker.ld"
do
    if [[ -f "$candidate" ]]; then
        LINKER_SCRIPT="$candidate"
        break
    fi
done

if [[ -z "$LINKER_SCRIPT" ]]; then
    echo "erro: apps.linker.ld não encontrado." >&2
    echo "      procurei em:" >&2
    echo "        $BASE_DIR/apps.linker.ld" >&2
    echo "        $APPS_DIR/apps.linker.ld" >&2
    echo "        $APPS_DIR/linker.ld" >&2
    echo "        $SCRIPT_DIR/apps.linker.ld" >&2
    exit 1
fi

# ------------------------------------------------------------
#  Argumento
# ------------------------------------------------------------
if [[ $# -lt 1 ]]; then
    echo "uso: $(basename "$0") <app>" >&2
    exit 1
fi

ARG="$1"

TARGET=""
if [[ -d "$APPS_CODES_DIR/$ARG" ]]; then
    TARGET="$APPS_CODES_DIR/$ARG"
elif [[ -d "$ARG" ]]; then
    TARGET="$ARG"
elif [[ -f "$APPS_CODES_DIR/$ARG" ]]; then
    TARGET="$APPS_CODES_DIR/$ARG"
elif [[ -f "$ARG" ]]; then
    TARGET="$ARG"
else
    echo "erro: '$ARG' não encontrado (nem em $APPS_CODES_DIR)." >&2
    exit 1
fi

# ------------------------------------------------------------
#  Resolve fonte + nome do app
# ------------------------------------------------------------
SRC=""
if [[ -d "$TARGET" ]]; then
    APP_NAME="$(basename "$TARGET")"

    for ext in c cpp c++ asm s; do
        if [[ -f "$TARGET/main.$ext" ]]; then
            SRC="$TARGET/main.$ext"
            break
        fi
    done

    if [[ -z "$SRC" ]]; then
        mapfile -t FOUND < <(find "$TARGET" -maxdepth 1 -type f \
            \( -name '*.c' -o -name '*.cpp' -o -name '*.c++' \
               -o -name '*.asm' -o -name '*.s' \) | sort)
        if [[ ${#FOUND[@]} -eq 0 ]]; then
            echo "erro: nenhum fonte suportado em $TARGET" >&2
            exit 1
        elif [[ ${#FOUND[@]} -gt 1 ]]; then
            echo "erro: vários fontes em $TARGET e nenhum main.* — ambíguo." >&2
            printf '      %s\n' "${FOUND[@]}" >&2
            exit 1
        fi
        SRC="${FOUND[0]}"
    fi
else
    SRC="$TARGET"
    APP_NAME="$(basename "$SRC")"
    APP_NAME="${APP_NAME%.*}"
fi

NAME="$(basename "$SRC")"
EXT_LOWER="$(printf '%s' "${NAME##*.}" | tr '[:upper:]' '[:lower:]')"

OBJ="$APPS_BUILD_DIR/$APP_NAME.o"
ELF="$APPS_BIN_DIR/$APP_NAME.elf"

echo ">> app     : $APP_NAME"
echo ">> fonte   : $SRC"
echo ">> linker  : $LINKER_SCRIPT"
echo ">> saída   : $ELF"
echo

# ------------------------------------------------------------
#  Flags
# ------------------------------------------------------------
CFLAGS=(
    -m32 -ffreestanding -fno-pie -fno-stack-protector
    -fno-builtin -nostdlib -nostartfiles
    -Wall -Wextra -O2
)
CXXFLAGS=(
    -m32 -ffreestanding -fno-pie -fno-stack-protector
    -fno-builtin -nostdlib -nostartfiles
    -fno-exceptions -fno-rtti
    -Wall -Wextra -O2
)

# ------------------------------------------------------------
#  Compila
# ------------------------------------------------------------
case "$EXT_LOWER" in
    c)
        echo ">> gcc -c $NAME"
        gcc "${CFLAGS[@]}" -c "$SRC" -o "$OBJ"
        ;;
    cpp|c++)
        echo ">> g++ -c $NAME"
        g++ "${CXXFLAGS[@]}" -c "$SRC" -o "$OBJ"
        ;;
    asm)
        command -v nasm >/dev/null 2>&1 || { echo "erro: nasm ausente" >&2; exit 1; }
        echo ">> nasm -f elf32 $NAME"
        nasm -f elf32 -o "$OBJ" "$SRC"
        ;;
    s)
        echo ">> gcc -m32 -c $NAME"
        gcc -m32 -c "$SRC" -o "$OBJ"
        ;;
    *)
        echo "erro: extensão não suportada: .$EXT_LOWER" >&2
        exit 1
        ;;
esac

# ------------------------------------------------------------
#  crt0 (opcional)
# ------------------------------------------------------------
CRT0_OBJ=""
if [[ "${NO_CRT0:-0}" != "1" && "$APP_NAME" != "crt0" ]]; then
    for search_dir in "$(dirname "$SRC")" "$APPS_CODES_DIR"; do
        for crt_ext in S s asm; do
            crt_src="$search_dir/crt0.$crt_ext"
            if [[ -f "$crt_src" ]]; then
                CRT0_OBJ="$APPS_BUILD_DIR/crt0.o"
                if [[ ! -f "$CRT0_OBJ" || "$crt_src" -nt "$CRT0_OBJ" ]]; then
                    echo ">> compilando crt0: $crt_src"
                    case "$crt_ext" in
                        asm) nasm -f elf32 -o "$CRT0_OBJ" "$crt_src" ;;
                        S|s) gcc -m32 -c "$crt_src" -o "$CRT0_OBJ" ;;
                    esac
                fi
                break 2
            fi
        done
    done
fi

# ------------------------------------------------------------
#  Link -> ELF32
# ------------------------------------------------------------
LINK_INPUTS=("$OBJ")
[[ -n "$CRT0_OBJ" ]] && LINK_INPUTS+=("$CRT0_OBJ")

echo ">> ld -> $ELF"
ld -m elf_i386 \
   -T "$LINKER_SCRIPT" \
   -static -no-pie -nostdlib \
   -o "$ELF" \
   "${LINK_INPUTS[@]}"

# ------------------------------------------------------------
#  Verificação (se readelf existir)
# ------------------------------------------------------------
if command -v readelf >/dev/null 2>&1; then
    echo
    echo ">> layout (readelf -l):"
    readelf -lW "$ELF" | sed 's/^/   /'
fi

# ------------------------------------------------------------
#  Limpa o .o intermediário (opcional)
# ------------------------------------------------------------
if [[ "${KEEP_OBJ:-1}" == "0" ]]; then
    rm -f "$OBJ"
fi

SIZE="$(wc -c < "$ELF" | tr -d ' ')"
echo
echo ">> OK: $ELF ($SIZE bytes)"