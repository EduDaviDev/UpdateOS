#!/usr/bin/env bash
# ============================================================
#  mkdisk.sh — cria uma imagem de disco pro UpOS (sem mtools).
#
#  Uso:
#     mkdisk.sh <name> <sizeMB> <fs> <contentdir>
#
#  Argumentos:
#     name        nome do disco → assets/Disks/Images/<name>.img
#     sizeMB      tamanho em MiB
#     fs          FAT12 | FAT16 | FAT32 | ExFAT
#     contentdir  subpasta de assets/Disks/Data/
#
#  Requer sudo (loop device + mount).
#
#  Dependências:
#     FAT:   sudo apt install dosfstools
#     ExFAT: sudo apt install exfatprogs
# ============================================================

set -euo pipefail

# ------------------------------------------------------------
#  Localização
# ------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"

DISKS_DIR="$BASE_DIR/assets/Disks"
DISKS_DATA_DIR="$DISKS_DIR/Data"
DISKS_IMGS_DIR="$DISKS_DIR/Images"

# ------------------------------------------------------------
#  Helpers
# ------------------------------------------------------------
die()  { echo "erro: $*" >&2; exit 1; }
info() { echo ">> $*"; }

usage() {
    cat >&2 <<EOF
uso: $(basename "$0") <name> <sizeMB> <fs> <contentdir>

  name        nome do disco (vira <name>.img)
  sizeMB      tamanho em megabytes (inteiro > 0)
  fs          FAT12 | FAT16 | FAT32 | ExFAT
  contentdir  subpasta de assets/Disks/Data/
EOF
    exit 1
}

# ------------------------------------------------------------
#  Args
# ------------------------------------------------------------
[[ $# -eq 4 ]] || usage

NAME="$1"
SIZE_MB="$2"
FS_RAW="$3"
CONTENT="$4"

[[ -n "$NAME" ]] || die "nome vazio"
[[ "$SIZE_MB" =~ ^[0-9]+$ ]] || die "sizeMB deve ser inteiro (recebido: $SIZE_MB)"
(( SIZE_MB > 0 )) || die "sizeMB deve ser > 0"
[[ -n "$CONTENT" ]] || die "contentdir vazio"

# Normaliza o FS: aceita "fat32", "FAT-32", "32", "exfat", etc.
FS_NORM="$(printf '%s' "$FS_RAW" | tr '[:lower:]' '[:upper:]' | tr -d ' -')"

case "$FS_NORM" in
    FAT12|12)  FS="FAT12" ;;
    FAT16|16)  FS="FAT16" ;;
    FAT32|32)  FS="FAT32" ;;
    EXFAT)     FS="EXFAT" ;;
    *) die "fs inválido: '$FS_RAW' (use FAT12, FAT16, FAT32 ou ExFAT)" ;;
esac

SRC_DIR="$DISKS_DATA_DIR/$CONTENT"
OUT_IMG="$DISKS_IMGS_DIR/$NAME.img"

[[ -d "$SRC_DIR" ]] || die "diretório de conteúdo não existe: $SRC_DIR"

mkdir -p "$DISKS_IMGS_DIR"

# ------------------------------------------------------------
#  Sanidade de tamanho por FS
# ------------------------------------------------------------
case "$FS" in
    FAT12)
        (( SIZE_MB <= 32 )) || die "FAT12 não suporta volumes > ~32 MiB (pedido: ${SIZE_MB} MiB)"
        ;;
    FAT16)
        (( SIZE_MB <= 4096 )) || die "FAT16 não suporta volumes > 4 GiB (pedido: ${SIZE_MB} MiB)"
        ;;
    FAT32)
        (( SIZE_MB >= 33 )) || die "FAT32 precisa de >= ~33 MiB. Use FAT16 pra volumes menores."
        ;;
esac

# ------------------------------------------------------------
#  Checagem de ferramentas
# ------------------------------------------------------------
if [[ "$FS" == "EXFAT" ]]; then
    command -v mkfs.exfat >/dev/null 2>&1 \
        || die "ExFAT requer 'mkfs.exfat'. Instale: sudo apt install exfatprogs"
else
    command -v mkfs.vfat >/dev/null 2>&1 \
        || die "FAT requer 'mkfs.vfat'. Instale: sudo apt install dosfstools"
fi

# ------------------------------------------------------------
#  Resumo
# ------------------------------------------------------------
info "nome     : $NAME"
info "tamanho  : ${SIZE_MB} MiB"
info "fs       : $FS"
info "conteúdo : $SRC_DIR"
info "saída    : $OUT_IMG"
echo

# ------------------------------------------------------------
#  1. Cria arquivo vazio
# ------------------------------------------------------------
info "criando arquivo de ${SIZE_MB} MiB..."
dd if=/dev/zero of="$OUT_IMG" bs=1M count="$SIZE_MB" status=none

# ------------------------------------------------------------
#  2. Label
# ------------------------------------------------------------
LABEL="$(printf '%s' "$NAME" | tr '[:lower:]' '[:upper:]' \
                          | tr -cd 'A-Z0-9_' \
                          | cut -c1-11)"
[[ -n "$LABEL" ]] || LABEL="UPOS"

# ------------------------------------------------------------
#  3. Loop device + traps de limpeza
# ------------------------------------------------------------
LOOP=""
MNT=""

cleanup() {
    if [[ -n "$MNT" ]]; then
        sudo umount "$MNT" 2>/dev/null || true
        rmdir "$MNT" 2>/dev/null || true
    fi
    if [[ -n "$LOOP" ]]; then
        sudo losetup -d "$LOOP" 2>/dev/null || true
    fi
}
trap cleanup EXIT

LOOP="$(sudo losetup --find --show "$OUT_IMG")"
info "loop device: $LOOP"

# ------------------------------------------------------------
#  4. Formata
# ------------------------------------------------------------
info "formatando ($FS)..."

case "$FS" in
    FAT12) sudo mkfs.vfat -F 12 -n "$LABEL" "$LOOP" >/dev/null ;;
    FAT16) sudo mkfs.vfat -F 16 -n "$LABEL" "$LOOP" >/dev/null ;;
    FAT32) sudo mkfs.vfat -F 32 -n "$LABEL" "$LOOP" >/dev/null ;;
    EXFAT) sudo mkfs.exfat -n "$LABEL" "$LOOP"        >/dev/null ;;
esac

# ------------------------------------------------------------
#  5. Copia conteúdo
# ------------------------------------------------------------
shopt -s nullglob dotglob
ENTRIES=("$SRC_DIR"/*)
shopt -u nullglob dotglob

if [[ ${#ENTRIES[@]} -eq 0 ]]; then
    info "diretório de conteúdo vazio — imagem só formatada"
else
    info "copiando ${#ENTRIES[@]} item(ns)..."

    MNT="$(mktemp -d)"
    sudo mount "$LOOP" "$MNT"

    # cp com sudo porque a imagem é root-montada
    sudo cp -r "$SRC_DIR"/. "$MNT"/

    # garante que tudo foi pro disco antes de desmontar
    sync

    sudo umount "$MNT"
    rmdir "$MNT"
    MNT=""
fi

# ------------------------------------------------------------
#  6. Verificação final
# ------------------------------------------------------------
SIZE_BYTES="$(stat -c%s "$OUT_IMG")"
echo
info "OK: $OUT_IMG ($(( SIZE_BYTES / 1024 / 1024 )) MiB)"