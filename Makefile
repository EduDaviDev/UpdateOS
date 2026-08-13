# ============================================================
# UpdateOS - Makefile principal (versão estável, sem warnings)
# ============================================================

# ------------------------------------------------------------
# Compiladores e flags
# ------------------------------------------------------------
CC       = gcc
CXX      = g++
AS       = nasm
LD       = ld
OBJCOPY  = objcopy

CFLAGS   = -m32 -ffreestanding -nostdlib -fno-pie -Wall -Wextra \
           -I./system/libs -I./system/drivers -I./system/interrupt
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS  = -f elf32
LDFLAGS  = -m elf_i386 -T system/kernel/link.ld

UTIL_CFLAGS   = -Wall -O2
UTIL_LDFLAGS  = -lncurses

# ------------------------------------------------------------
# Diretórios
# ------------------------------------------------------------
BUILD_DIR      = build
KERNEL_OBJ_DIR = $(BUILD_DIR)/kernel
ISO_DIR        = $(BUILD_DIR)/iso
DISK_IMG_DIR   = $(BUILD_DIR)/disks
TOOLS_DIR      = tools
TOOLS_BIN      = $(TOOLS_DIR)/bin

# ------------------------------------------------------------
# Utilitários (compilados para tools/bin/)
# ------------------------------------------------------------
MENU_SRC       = $(TOOLS_DIR)/menu.c
MENU_BIN       = $(TOOLS_BIN)/menu

MAKEAPPS_SRC   = $(TOOLS_DIR)/makeapps.c
MAKEAPPS_BIN   = $(TOOLS_BIN)/makeapps

READCONF_SRC   = $(TOOLS_DIR)/readconf.c
READCONF_BIN   = $(TOOLS_BIN)/readconf

# ------------------------------------------------------------
# Arquivos do kernel
# ------------------------------------------------------------
KERNEL_ELF     = $(BUILD_DIR)/UpdateOS.elf
KERNEL_BIN     = $(BUILD_DIR)/UpdateOS.bin
ISO_FILE       = $(BUILD_DIR)/UpdateOS.iso

# ------------------------------------------------------------
# Configurações de disco (podem ser sobrescritas)
# ------------------------------------------------------------
DISK_PATH    ?= disk_contents
MULTI_FS     ?= false
FS_LIST      ?= fat32
DISK_SIZE_MB ?= 64

# ------------------------------------------------------------
# Configurações de Apps (UEX)
# ------------------------------------------------------------
APP_TYPE     ?= coded
APPS_BASE    ?= apps
APPS_OBJ_DIR ?= $(APPS_BASE)/compiled
APPS_UEX_DIR ?= $(APPS_BASE)/uex

APPS_CODED_PATH = $(APPS_BASE)/coded
APPS_BIN_PATH   = $(APPS_BASE)/binaries
APPS_OBJ_PATH   = $(APPS_BASE)/objects

ifeq ($(APP_TYPE), multi)
    APPS_PATH = $(APPS_BASE)
else
    APPS_PATH = $(APPS_BASE)/$(APP_TYPE)
endif

STACK_SIZE   ?= 1024
ENTRY_OFFSET ?= 0x10

# ------------------------------------------------------------
# Fontes do kernel
# ------------------------------------------------------------
C_SRCS   = $(shell find system -name '*.c' ! -name 'keyboard_irq.c' ! -name 'kernel_example.c')
ASM_SRCS = $(shell find system -name '*.asm' ! -name 'isr_handlers.asm' ! -name 'irq_handlers.asm')
ASM_SRCS += system/interrupt/irq_handlers.asm

OBJS = $(patsubst system/%.c, $(KERNEL_OBJ_DIR)/%.o, $(C_SRCS)) \
       $(patsubst system/%.asm, $(KERNEL_OBJ_DIR)/%.o, $(ASM_SRCS))

# ------------------------------------------------------------
# Apps: coleta de fontes
# ------------------------------------------------------------
ifeq ($(APP_TYPE), coded)
    APP_SRCS = $(shell find $(APPS_PATH) -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.c++' -o -name '*.asm' \) 2>/dev/null)
    APP_OBJS = $(patsubst $(APPS_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.c,%.o, $(filter %.c, $(APP_SRCS))))
    APP_OBJS += $(patsubst $(APPS_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.cpp,%.o, $(filter %.cpp, $(APP_SRCS))))
    APP_OBJS += $(patsubst $(APPS_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.c++,%.o, $(filter %.c++, $(APP_SRCS))))
    APP_OBJS += $(patsubst $(APPS_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.asm,%.o, $(filter %.asm, $(APP_SRCS))))
endif

ifeq ($(APP_TYPE), binaries)
    APP_BINS = $(shell find $(APPS_PATH) -type f -name '*.bin' 2>/dev/null)
    APP_UEX_FILES = $(patsubst $(APPS_PATH)/%, $(APPS_UEX_DIR)/%, $(patsubst %.bin,%.uex, $(APP_BINS)))
endif

ifeq ($(APP_TYPE), objects)
    APP_OBJS = $(shell find $(APPS_PATH) -type f -name '*.o' 2>/dev/null)
endif

ifeq ($(APP_TYPE), multi)
    APP_CODED_SRCS = $(shell find $(APPS_CODED_PATH) -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.c++' -o -name '*.asm' \) 2>/dev/null)
    APP_OBJS += $(patsubst $(APPS_CODED_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.c,%.o, $(filter %.c, $(APP_CODED_SRCS))))
    APP_OBJS += $(patsubst $(APPS_CODED_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.cpp,%.o, $(filter %.cpp, $(APP_CODED_SRCS))))
    APP_OBJS += $(patsubst $(APPS_CODED_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.c++,%.o, $(filter %.c++, $(APP_CODED_SRCS))))
    APP_OBJS += $(patsubst $(APPS_CODED_PATH)/%, $(APPS_OBJ_DIR)/%, $(patsubst %.asm,%.o, $(filter %.asm, $(APP_CODED_SRCS))))
    APP_BINS = $(shell find $(APPS_BIN_PATH) -type f -name '*.bin' 2>/dev/null)
    APP_UEX_FILES += $(patsubst $(APPS_BIN_PATH)/%, $(APPS_UEX_DIR)/%, $(patsubst %.bin,%.uex, $(APP_BINS)))
    APP_OBJS += $(shell find $(APPS_OBJ_PATH) -type f -name '*.o' 2>/dev/null)
endif

APP_UEX_FILES += $(patsubst $(APPS_OBJ_DIR)/%.o, $(APPS_UEX_DIR)/%.uex, $(APP_OBJS))

# ============================================================
# Alvos principais (apenas UMA definição de cada)
# ============================================================
.PHONY: all build system menu run-menu apps apps_nv disks clean run iso help

all: menu

# Esta é a ÚNICA definição da regra 'build'
build: $(KERNEL_BIN)
	@echo "Kernel compilado em $(KERNEL_BIN)"

system: $(KERNEL_BIN) iso disks
	@echo "Executando UpdateOS no QEMU..."
	@cmd="qemu-system-x86_64 -serial stdio -cdrom $(ISO_FILE) -boot d"; \
	for img in $(DISK_IMG_DIR)/*.img; do \
		if [ -f "$$img" ]; then \
			cmd="$$cmd -drive file=$$img,format=raw,if=ide"; \
		fi; \
	done; \
	echo "Comando: $$cmd"; \
	cmd.exe /c "$$cmd"

menu: $(MENU_BIN)
	@echo "Menu compilado em $(MENU_BIN)"
	./$(MENU_BIN)

run-menu: menu

# ------------------------------------------------------------
# apps – lê .cfg, sobrescreve variáveis e chama apps_nv
# ------------------------------------------------------------
apps: $(MAKEAPPS_BIN) $(READCONF_BIN)
	@if [ -z "$(APP_UEX_FILES)" ]; then \
		echo "Nenhum arquivo fonte encontrado em $(APPS_PATH)"; \
		exit 1; \
	fi
	@cfg_file=$$(find $(APPS_BASE) -type f -name "app.cfg" 2>/dev/null | head -1); \
	if [ -n "$$cfg_file" ]; then \
		config=$$($(READCONF_BIN) "$$cfg_file"); \
		if [ -n "$$config" ]; then \
			STACK_SIZE=$$(echo "$$config" | grep -o 'STACK_SIZE=[0-9]*' | cut -d= -f2); \
			ENTRY_OFFSET=$$(echo "$$config" | grep -o 'ENTRY_OFFSET=[0-9]*' | cut -d= -f2); \
		fi; \
	fi; \
	STACK_SIZE=$${STACK_SIZE:-$(STACK_SIZE)}; \
	ENTRY_OFFSET=$${ENTRY_OFFSET:-$(ENTRY_OFFSET)}; \
	$(MAKE) apps_nv STACK_SIZE=$$STACK_SIZE ENTRY_OFFSET=$$ENTRY_OFFSET

apps_nv: $(MAKEAPPS_BIN) $(APP_UEX_FILES)
	@if [ -z "$(APP_UEX_FILES)" ]; then \
		echo "Nenhum arquivo fonte encontrado em $(APPS_PATH)"; \
		exit 1; \
	fi
	@echo "Apps compilados! Arquivos .uex em $(APPS_UEX_DIR)"
	@echo "Tipo: $(APP_TYPE) | Path: $(APPS_PATH)"

# ------------------------------------------------------------
# Discos
# ------------------------------------------------------------
disks:
	@echo "Criando discos com configuração:"
	@echo "  Path: $(DISK_PATH)"
	@echo "  Multi-FS: $(MULTI_FS)"
	@echo "  FS List: $(FS_LIST)"
	@echo "  Tamanho: $(DISK_SIZE_MB) MiB"
	@mkdir -p $(DISK_IMG_DIR)
ifeq ($(MULTI_FS), true)
	@for fs in $(FS_LIST); do \
		echo "Processando $$fs..."; \
		content_dir="$(DISK_PATH)/$$fs"; \
		mkdir -p "$$content_dir"; \
		if [ -z "$$(ls -A "$$content_dir" 2>/dev/null)" ]; then \
			echo "Criando arquivo de exemplo em $$content_dir/hello.txt"; \
			echo "Hello from $$fs filesystem!" > "$$content_dir/hello.txt"; \
		fi; \
		img="$(DISK_IMG_DIR)/$$fs.img"; \
		dd if=/dev/zero of=$$img bs=1M count=$(DISK_SIZE_MB) status=progress; \
		case "$$fs" in \
			fat12) fat_type=12 ;; \
			fat16) fat_type=16 ;; \
			fat32) fat_type=32 ;; \
			exfat) fat_type=32 ;; \
			*) fat_type=32 ;; \
		esac; \
		if [ "$$fs" = "exfat" ]; then \
			mkfs.exfat -I $$img; \
		else \
			mkfs.fat -F $$fat_type -I $$img; \
		fi; \
		mcopy -s -i $$img "$$content_dir"/* ::/ 2>/dev/null || echo "Nenhum arquivo para copiar em $$fs"; \
		echo "Imagem $$fs criada em $$img"; \
	done
else
	@content_dir="$(DISK_PATH)"; \
	mkdir -p "$$content_dir"; \
	if [ -z "$$(ls -A "$$content_dir" 2>/dev/null)" ]; then \
		echo "Criando arquivo de exemplo em $$content_dir/hello.txt"; \
		echo "Hello from FAT32!" > "$$content_dir/hello.txt"; \
	fi; \
	img="$(DISK_IMG_DIR)/disk.img"; \
	dd if=/dev/zero of=$$img bs=1M count=$(DISK_SIZE_MB) status=progress; \
	mkfs.fat -F 32 -I $$img; \
	mcopy -s -i $$img "$$content_dir"/* ::/ 2>/dev/null || echo "Nenhum arquivo para copiar"; \
	echo "Imagem única criada em $$img"
endif
	@echo "Discos criados em $(DISK_IMG_DIR)"

# ------------------------------------------------------------
# ISO
# ------------------------------------------------------------
iso: $(ISO_FILE)
	@echo "ISO criada em $(ISO_FILE)"

# ------------------------------------------------------------
# Execução (QEMU)
# ------------------------------------------------------------
run: iso disks
	@echo "Executando UpdateOS no QEMU..."
	@cmd="qemu-system-x86_64 -serial stdio -cdrom $(ISO_FILE) -boot d"; \
	for img in $(DISK_IMG_DIR)/*.img; do \
		if [ -f "$$img" ]; then \
			cmd="$$cmd -drive file=$$img,format=raw,if=ide"; \
		fi; \
	done; \
	echo "Comando: $$cmd"; \
	cmd.exe /c "$$cmd"

# ------------------------------------------------------------
# Limpeza
# ------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR) $(APPS_OBJ_DIR) $(APPS_UEX_DIR) $(TOOLS_BIN)
	@echo "Limpeza concluída!"

clean-apps:
	rm -rf $(APPS_OBJ_DIR) $(APPS_UEX_DIR)
	@echo "Apps limpos."

# ------------------------------------------------------------
# Ajuda
# ------------------------------------------------------------
help:
	@echo "Comandos disponíveis:"
	@echo "  make all                - Compila o kernel (sem executar)"
	@echo "  make build              - Alias para make all"
	@echo "  make system             - Compila o kernel, cria ISO e discos, e executa no QEMU"
	@echo "  make menu               - Compila e executa o menu interativo (tools/menu.c)"
	@echo "  make run-menu           - Alias para make menu"
	@echo "  make apps [OPÇÕES]      - Compila apps (valida e chama apps_nv)"
	@echo "  make disks [OPÇÕES]     - Cria imagens de disco"
	@echo "  make iso                - Cria a ISO"
	@echo "  make run                - Executa o QEMU (compila ISO e discos se necessário)"
	@echo "  make clean              - Remove todos os arquivos compilados"
	@echo "  make clean-apps         - Remove apenas apps compilados"
	@echo ""
	@echo "Opções para 'make apps':"
	@echo "  STACK_SIZE=X   - tamanho da pilha (padrão 1024)"
	@echo "  ENTRY_OFFSET=Y - offset do código (padrão 0x10)"
	@echo "  APP_TYPE=T     - coded, binaries, objects, multi (padrão coded)"
	@echo "  APPS_BASE=P    - diretório base dos apps (padrão ./apps)"
	@echo ""
	@echo "Exemplos:"
	@echo "  make apps STACK_SIZE=4096 ENTRY_OFFSET=32"
	@echo "  make apps APP_TYPE=multi APPS_BASE=./meus_apps"

# ============================================================
# Regras para compilar utilitários (em tools/bin/)
# ============================================================

$(TOOLS_BIN):
	mkdir -p $@

$(MENU_BIN): $(MENU_SRC) | $(TOOLS_BIN)
	$(CC) $(UTIL_CFLAGS) -o $@ $< $(UTIL_LDFLAGS)

$(MAKEAPPS_BIN): $(MAKEAPPS_SRC) | $(TOOLS_BIN)
	$(CC) $(UTIL_CFLAGS) -o $@ $<

$(READCONF_BIN): $(READCONF_SRC) | $(TOOLS_BIN)
	$(CC) $(UTIL_CFLAGS) -o $@ $<

# ============================================================
# Regras para o kernel
# ============================================================

$(BUILD_DIR) $(KERNEL_OBJ_DIR):
	mkdir -p $@

$(KERNEL_OBJ_DIR)/%.o: system/%.c | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_OBJ_DIR)/%.o: system/%.asm | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	cp $< $@

# ============================================================
# Regras para os apps (compilação e geração UEX)
# ============================================================

$(APPS_OBJ_DIR):
	mkdir -p $@

$(APPS_UEX_DIR):
	mkdir -p $@

# Compilação de objetos .o (coded)
$(APPS_OBJ_DIR)/%.o: $(APPS_PATH)/%.c | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) -m32 -ffreestanding -fno-pie -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_PATH)/%.cpp | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_PATH)/%.c++ | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_PATH)/%.asm | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@

# Para multi (usa APPS_CODED_PATH)
ifneq ($(APP_TYPE), coded)
$(APPS_OBJ_DIR)/%.o: $(APPS_CODED_PATH)/%.c | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) -m32 -ffreestanding -fno-pie -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_CODED_PATH)/%.cpp | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_CODED_PATH)/%.c++ | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -c $< -o $@

$(APPS_OBJ_DIR)/%.o: $(APPS_CODED_PATH)/%.asm | $(APPS_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@
endif

# Gera .uex a partir de .o (usando makeapps)
$(APPS_UEX_DIR)/%.uex: $(APPS_OBJ_DIR)/%.o | $(APPS_UEX_DIR) $(MAKEAPPS_BIN)
	@echo "Gerando UEX para $* com makeapps..."
	@mkdir -p $(dir $@)
	@base_name=$$(basename "$*"); \
	cfg_file=$$(find $(APPS_BASE) -type f -name "$$base_name.cfg" 2>/dev/null | head -1); \
	if [ -n "$$cfg_file" ]; then \
		stack_size=$$(grep -E '^[[:space:]]*STACK_SIZE[[:space:]]*=' $$cfg_file | head -1 | sed 's/^[[:space:]]*STACK_SIZE[[:space:]]*=[[:space:]]*//'); \
		entry_offset=$$(grep -E '^[[:space:]]*ENTRY_OFFSET[[:space:]]*=' $$cfg_file | head -1 | sed 's/^[[:space:]]*ENTRY_OFFSET[[:space:]]*=[[:space:]]*//'); \
	fi; \
	stack_size=$${stack_size:-$(STACK_SIZE)}; \
	entry_offset=$${entry_offset:-$(ENTRY_OFFSET)}; \
	$(LD) -m elf_i386 -Ttext 0x0 --entry=_start -o $(APPS_OBJ_DIR)/$*.elf $<; \
	$(OBJCOPY) -O binary $(APPS_OBJ_DIR)/$*.elf $(APPS_OBJ_DIR)/$*.bin; \
	$(MAKEAPPS_BIN) --stack $$stack_size --entry $$entry_offset \
	                --input $(APPS_OBJ_DIR)/$*.bin \
	                --output $@; \
	rm -f $(APPS_OBJ_DIR)/$*.elf $(APPS_OBJ_DIR)/$*.bin
	@echo "UEX criado: $@"

# Gera .uex a partir de .bin (binários prontos)
$(APPS_UEX_DIR)/%.uex: $(APPS_BIN_PATH)/%.bin | $(APPS_UEX_DIR) $(MAKEAPPS_BIN)
	@echo "Gerando UEX para $* (binário pronto) com makeapps..."
	@mkdir -p $(dir $@)
	@base_name=$$(basename "$*"); \
	cfg_file=$$(find $(APPS_BASE) -type f -name "$$base_name.cfg" 2>/dev/null | head -1); \
	if [ -n "$$cfg_file" ]; then \
		stack_size=$$(grep -E '^[[:space:]]*STACK_SIZE[[:space:]]*=' $$cfg_file | head -1 | sed 's/^[[:space:]]*STACK_SIZE[[:space:]]*=[[:space:]]*//'); \
		entry_offset=$$(grep -E '^[[:space:]]*ENTRY_OFFSET[[:space:]]*=' $$cfg_file | head -1 | sed 's/^[[:space:]]*ENTRY_OFFSET[[:space:]]*=[[:space:]]*//'); \
	fi; \
	stack_size=$${stack_size:-$(STACK_SIZE)}; \
	entry_offset=$${entry_offset:-$(ENTRY_OFFSET)}; \
	$(MAKEAPPS_BIN) --stack $$stack_size --entry $$entry_offset \
	                --input $< \
	                --output $@

# ============================================================
# Regras para ISO
# ============================================================

$(ISO_DIR)/boot/grub:
	mkdir -p $@

$(ISO_DIR)/boot/UpdateOS.bin: $(KERNEL_BIN) | $(ISO_DIR)/boot/grub
	cp $< $@

$(ISO_DIR)/boot/grub/grub.cfg: system/kernel/grub.cfg | $(ISO_DIR)/boot/grub
	cp $< $@

$(ISO_FILE): $(ISO_DIR)/boot/UpdateOS.bin $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR)