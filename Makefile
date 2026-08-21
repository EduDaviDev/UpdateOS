# ============================================================
# UpdateOS - Makefile (versão final com menu e correções)
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
           -I./system/libs -I./system/drivers -I./system/interrupt \
           -I./system/memory
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS  = -f elf32
LDFLAGS  = -m elf_i386 -T system/kernel/link.ld

# ------------------------------------------------------------
# Diretórios
# ------------------------------------------------------------
BUILD_DIR      = build
KERNEL_OBJ_DIR = $(BUILD_DIR)/kernel
ISO_DIR        = $(BUILD_DIR)/iso
DISK_IMG_DIR   = $(BUILD_DIR)/disks
DISK_CONTENTS  = disk_contents
APPS_BIN_DIR   = $(BUILD_DIR)/apps_bin
TOOLS_DIR      = tools
TOOLS_BIN      = $(TOOLS_DIR)/bin

# ------------------------------------------------------------
# Arquivos do kernel
# ------------------------------------------------------------
KERNEL_ELF     = $(BUILD_DIR)/UpdateOS.elf
KERNEL_BIN     = $(BUILD_DIR)/UpdateOS.bin
ISO_FILE       = $(BUILD_DIR)/UpdateOS.iso

# ------------------------------------------------------------
# Fontes do kernel (todos os arquivos em system/)
# ------------------------------------------------------------
C_SRCS   = $(shell find system -name '*.c')
CPP_SRCS = $(shell find system -name '*.cpp' -o -name '*.c++' 2>/dev/null)

# Compila TODOS os .asm (sem exclusões)
ASM_SRCS = $(shell find system -name '*.asm')

OBJS = $(patsubst system/%.c, $(KERNEL_OBJ_DIR)/%.o, $(C_SRCS)) \
       $(patsubst system/%.cpp, $(KERNEL_OBJ_DIR)/%.o, $(CPP_SRCS)) \
       $(patsubst system/%.c++, $(KERNEL_OBJ_DIR)/%.o, $(CPP_SRCS)) \
       $(patsubst system/%.asm, $(KERNEL_OBJ_DIR)/%.o, $(ASM_SRCS))

# ------------------------------------------------------------
# Apps (flat binaries)
# ------------------------------------------------------------
APPS_SRC_DIR  ?= apps

APP_ASM_SRCS = $(shell find $(APPS_SRC_DIR) -name '*.asm' 2>/dev/null)
APP_BINS     = $(patsubst $(APPS_SRC_DIR)/%.asm, $(APPS_BIN_DIR)/%, $(APP_ASM_SRCS))

# ------------------------------------------------------------
# Utilitário menu (host, com ncurses)
# ------------------------------------------------------------
MENU_SRC     = $(TOOLS_DIR)/menu.c
MENU_BIN     = $(TOOLS_DIR)/menu
UTIL_CFLAGS  = -Wall -O2
UTIL_LDFLAGS = -lncurses

# ------------------------------------------------------------
# Configuração de discos (multi-FS automático)
# ------------------------------------------------------------
DISK_PATH    ?= $(DISK_CONTENTS)
MULTI_FS     ?= false
FS_TYPES     := FAT12 FAT16 FAT32 ExFAT

# ============================================================
# Alvos principais
# ============================================================
.PHONY: all build system menu run iso apps disks clean help

all: menu

# Compila e executa o menu interativo
menu: $(MENU_BIN)
	@echo "Executando menu..."
	./$(MENU_BIN)

# system é um alias para run
system: $(KERNEL_BIN) run

# run: compila ISO, discos, apps e executa no QEMU
run:
	@echo "Executando UpdateOS no QEMU (logs em serial.log e qemu.log)..."
	@cmd="qemu-system-x86_64 -serial file:serial.log -cdrom $(ISO_FILE) -boot d -no-reboot -d int,cpu_reset -D qemu.log"; \
	for img in $(DISK_IMG_DIR)/*.img; do \
		if [ -f "$$img" ]; then \
			cmd="$$cmd -drive file=$$img,format=raw,if=ide"; \
		fi; \
	done; \
	echo "Comando: $$cmd"; \
	cmd.exe /c "$$cmd"

# iso: compila o kernel e gera a ISO
iso: $(ISO_FILE)

# apps: compila os apps e copia para disk_contents/
apps: $(APP_BINS)
	@echo "Apps compilados em $(APPS_BIN_DIR)"
	@mkdir -p $(DISK_CONTENTS)
	@cp $(APPS_BIN_DIR)/* $(DISK_CONTENTS)/ 2>/dev/null || true
	@echo "Binários copiados para $(DISK_CONTENTS)"

# disks: cria imagem(s) de disco (multi-FS ou único)
disks:
	@echo "Criando discos (MULTI_FS=$(MULTI_FS))..."
	@mkdir -p $(DISK_IMG_DIR)
ifeq ($(MULTI_FS), true)
	@for fs in $(FS_TYPES); do \
		content_dir="$(DISK_PATH)/$$fs"; \
		if [ -d "$$content_dir" ]; then \
			echo "Processando $$fs..."; \
			img="$(DISK_IMG_DIR)/$$fs.img"; \
			dd if=/dev/zero of=$$img bs=1M count=64 status=progress; \
			case "$$fs" in \
				FAT12) fat_type=12 ;; \
				FAT16) fat_type=16 ;; \
				FAT32) fat_type=32 ;; \
				ExFAT) fat_type=32 ;; \
				*) fat_type=32 ;; \
			esac; \
			if [ "$$fs" = "ExFAT" ]; then \
				mkfs.exfat -I $$img; \
			else \
				mkfs.fat -F $$fat_type -I $$img; \
			fi; \
			mcopy -s -i $$img "$$content_dir"/* ::/ 2>/dev/null || echo "Nenhum arquivo para copiar em $$fs"; \
			echo "Imagem $$fs criada em $$img"; \
		fi; \
	done
else
	@content_dir="$(DISK_PATH)"; \
	if [ ! -d "$$content_dir" ]; then \
		mkdir -p "$$content_dir"; \
		echo "Criando diretório $$content_dir"; \
	fi; \
	if [ -z "$$(ls -A "$$content_dir" 2>/dev/null)" ]; then \
		echo "Criando arquivo de exemplo em $$content_dir/hello.txt"; \
		echo "Hello from FAT32!" > "$$content_dir/hello.txt"; \
	fi; \
	img="$(DISK_IMG_DIR)/disk.img"; \
	dd if=/dev/zero of=$$img bs=1M count=64 status=progress; \
	mkfs.fat -F 32 -I $$img; \
	mcopy -s -i $$img "$$content_dir"/* ::/ 2>/dev/null || echo "Nenhum arquivo para copiar"; \
	echo "Imagem única criada em $$img"
endif
	@echo "Discos criados em $(DISK_IMG_DIR)"

# Limpeza
clean:
	rm -rf $(BUILD_DIR) $(APPS_BIN_DIR) $(DISK_IMG_DIR) $(TOOLS_BIN) $(MENU_BIN)
	@echo "Limpeza concluída!"

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make all            - Compila e executa o menu interativo"
	@echo "  make menu           - Compila e executa o menu"
	@echo "  make build          - Apenas compila o kernel"
	@echo "  make system         - Alias para 'make run'"
	@echo "  make run            - Compila ISO, discos, apps e executa no QEMU"
	@echo "  make iso            - Compila o kernel e gera a ISO"
	@echo "  make apps           - Compila os apps (flat binaries)"
	@echo "  make disks          - Cria a(s) imagem(ns) de disco"
	@echo "  make clean          - Remove arquivos compilados"
	@echo ""
	@echo "Variáveis para discos:"
	@echo "  MULTI_FS=true|false  - Ativa múltiplos sistemas de arquivos (padrão false)"
	@echo "  DISK_PATH=<dir>      - Diretório com conteúdo (padrão disk_contents)"

# ============================================================
# Regras para o kernel
# ============================================================
$(BUILD_DIR) $(KERNEL_OBJ_DIR):
	mkdir -p $@

$(KERNEL_OBJ_DIR)/%.o: system/%.c | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_OBJ_DIR)/%.o: system/%.cpp | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(KERNEL_OBJ_DIR)/%.o: system/%.c++ | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(KERNEL_OBJ_DIR)/%.o: system/%.asm | $(KERNEL_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	cp $< $@

# ============================================================
# Regras para apps (flat binaries)
# ============================================================
$(APPS_BIN_DIR):
	mkdir -p $@

$(APPS_BIN_DIR)/%: $(APPS_SRC_DIR)/%.asm | $(APPS_BIN_DIR)
	@mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@.o
	$(LD) -m elf_i386 -Ttext=0x400000 -o $@.elf $@.o
	$(OBJCOPY) -O binary $@.elf $@
	@rm -f $@.o $@.elf

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

# ============================================================
# Regra para o menu (utilitário host)
# ============================================================
$(MENU_BIN): $(MENU_SRC)
	$(CC) $(UTIL_CFLAGS) -o $@ $< $(UTIL_LDFLAGS)