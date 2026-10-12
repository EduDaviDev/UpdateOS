# ============================================================
#  UpOS Makefile
#  Targets/prereqs: sem aspas (relativos, sem espaços)
#  Receitas: todo caminho entre aspas
# ============================================================

SHELL := /bin/bash

# ---- Ferramentas ----
ASM   = nasm
CC    = gcc
CXX   = g++
LD    = ld
GRUB  = grub-mkrescue
QEMU  = qemu-system-i386

# Scripts externos (relativos — sem espaços)
MAKEAPPS = assets/Tools/Codes/makeapps.sh
MKDISK   = assets/Tools/Codes/mkdisk.sh

# ---- Flags (kernel) ----
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -nostdlib -fno-builtin \
           -fno-stack-protector -fno-pic -Wall -Wextra \
           -Ikernel -Ikernel/drivers -g -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  = -m elf_i386 -T kernel/linker.ld -O2

# ---- Caminho absoluto do projeto (pode ter espaços — usado SÓ dentro de receitas) ----
PROJECT_ROOT := $(CURDIR)

# ---- Diretórios de build (relativos, sem espaços) ----
KERNEL_DIR = kernel
BUILD_DIR  = build
OBJ_DIR    = $(BUILD_DIR)/kernel
DEP_DIR    = $(BUILD_DIR)/dependencies
ISO_DIR    = $(BUILD_DIR)/iso
BOOT_DIR   = $(ISO_DIR)/boot
GRUB_DIR   = $(BOOT_DIR)/grub
SYSTEM_DIR = $(ISO_DIR)/system
LOG_DIR    = logs
QEMU_LOG   = $(LOG_DIR)/qemu.log
SERIAL_LOG = $(LOG_DIR)/serial.log

# ---- Arquivos finais ----
KERNEL_ELF   = $(BUILD_DIR)/UpKnel.elf
ISO_FILE     = $(BUILD_DIR)/UpOS.iso
GRUB_CFG_SRC = $(KERNEL_DIR)/grub.cfg
GRUB_CFG_DST = $(GRUB_DIR)/grub.cfg

# ---- Fontes do kernel ----
C_SOURCES   := $(shell find $(KERNEL_DIR) -name '*.c' 2>/dev/null)
CXX_SOURCES := $(shell find $(KERNEL_DIR) -name '*.cpp' -o -name '*.c++' 2>/dev/null)
ASM_SOURCES := $(shell find $(KERNEL_DIR) -name '*.asm' 2>/dev/null)
GAS_SOURCES := $(shell find $(KERNEL_DIR) -name '*.s'  2>/dev/null)

C_OBJECTS   := $(patsubst $(KERNEL_DIR)/%.c,   $(OBJ_DIR)/%.o, $(C_SOURCES))
CXX_OBJECTS := $(patsubst $(KERNEL_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CXX_SOURCES))
CXX_OBJECTS := $(patsubst $(KERNEL_DIR)/%.c++, $(OBJ_DIR)/%.o, $(CXX_OBJECTS))
ASM_OBJECTS := $(patsubst $(KERNEL_DIR)/%.asm, $(OBJ_DIR)/%.o, $(ASM_SOURCES))
GAS_OBJECTS := $(patsubst $(KERNEL_DIR)/%.s,   $(OBJ_DIR)/%.o, $(GAS_SOURCES))
ALL_OBJECTS := $(C_OBJECTS) $(CXX_OBJECTS) $(ASM_OBJECTS) $(GAS_OBJECTS)

C_DEPENDS   := $(patsubst $(KERNEL_DIR)/%.c,   $(DEP_DIR)/%.d, $(C_SOURCES))
CXX_DEPENDS := $(patsubst $(KERNEL_DIR)/%.cpp, $(DEP_DIR)/%.d, $(CXX_SOURCES))
CXX_DEPENDS := $(patsubst $(KERNEL_DIR)/%.c++, $(DEP_DIR)/%.d, $(CXX_DEPENDS))
ALL_DEPENDS := $(C_DEPENDS) $(CXX_DEPENDS)

# ============================================================
.PHONY: all system kernel run debug clean help \
        app apps disk disks

# ============================================================
#  all — abre o MakeGUI
# ============================================================
all:
	@clear
	@chmod +x "$(MAKEAPPS)"
	@chmod +x "$(MKDISK)"
	@"$(PROJECT_ROOT)/.venv/bin/python3" \
		"$(PROJECT_ROOT)/assets/Tools/Codes/MakeGUI/main.py"

# ============================================================
#  Build
# ============================================================
system: $(ISO_FILE)
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(ALL_OBJECTS) $(KERNEL_DIR)/linker.ld
	@mkdir -p "$(dir $@)"
	"$(LD)" $(LDFLAGS) -o "$@" $(ALL_OBJECTS)

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p "$(dir $@)" "$(DEP_DIR)/$(dir $*)"
	"$(CC)" $(CFLAGS) -MMD -MP -MF "$(DEP_DIR)/$*.d" -c "$<" -o "$@"

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.cpp
	@mkdir -p "$(dir $@)" "$(DEP_DIR)/$(dir $*)"
	"$(CXX)" $(CXXFLAGS) -MMD -MP -MF "$(DEP_DIR)/$*.d" -c "$<" -o "$@"

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.c++
	@mkdir -p "$(dir $@)" "$(DEP_DIR)/$(dir $*)"
	"$(CXX)" $(CXXFLAGS) -MMD -MP -MF "$(DEP_DIR)/$*.d" -c "$<" -o "$@"

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p "$(dir $@)"
	"$(ASM)" $(ASMFLAGS) "$<" -o "$@"

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p "$(dir $@)"
	"$(CC)" $(CFLAGS) -c "$<" -o "$@"

$(ISO_FILE): $(KERNEL_ELF) $(GRUB_CFG_SRC)
	@mkdir -p "$(BOOT_DIR)" "$(GRUB_DIR)" "$(SYSTEM_DIR)"
	cp "$(KERNEL_ELF)" "$(BOOT_DIR)/upkernel.elf"
	cp "$(GRUB_CFG_SRC)" "$(GRUB_CFG_DST)"
	@if [ -d "$(KERNEL_DIR)/iso" ]; then \
		cp -r "$(KERNEL_DIR)/iso"/. "$(SYSTEM_DIR)/" 2>/dev/null || true; \
	fi
	"$(GRUB)" -o "$(ISO_FILE)" "$(ISO_DIR)"

# ============================================================
#  QEMU
#
#  Variáveis de controle do run:
#    QEMU_DISKS       lista separada por vírgula; se VAZIA, inclui
#                     todos os .img de assets/Disks/Images/
#    QEMU_EXTRA_FLAGS flags extras repassadas ao QEMU (opcional)
#
#  Exemplos:
#    make run
#    make run QEMU_DISKS=Disk0,Disk1
#    make run QEMU_EXTRA_FLAGS="-s -S"
#    make run QEMU_DISKS=Disk0 QEMU_EXTRA_FLAGS="-m 512M"
# ============================================================

QEMU_BASE_FLAGS = -cdrom "$(ISO_FILE)" -boot d \
	-display gtk \
	-rtc base=localtime \
	-no-reboot -no-shutdown \
	-vga std \
	-cpu max \
	-d int,cpu_reset \
	-D "$(QEMU_LOG)" \
	-serial "file:$(SERIAL_LOG)"

QEMU_DISKS       ?=
QEMU_EXTRA_FLAGS ?=

run:
	@mkdir -p "$(LOG_DIR)"
	@echo "==> QEMU log:   $(QEMU_LOG)"
	@echo "==> Serial log: $(SERIAL_LOG)"
	@discs='$(QEMU_DISKS)'; \
	imgs_dir='$(PROJECT_ROOT)/assets/Disks/Images'; \
	drive_flags=(); \
	attached=(); \
	idx=0; \
	if [ -z "$$discs" ]; then \
		shopt -s nullglob; \
		for img in "$$imgs_dir"/*.img; do \
			drive_flags+=(-drive "file=$$img,format=raw,if=ide,index=$$idx"); \
			attached+=("$$(basename "$$img" .img)"); \
			idx=$$((idx+1)); \
		done; \
	else \
		IFS=',' read -ra names <<< "$$discs"; \
		for name in "$${names[@]}"; do \
			name="$$(echo "$$name" | sed 's/^[[:space:]]*//; s/[[:space:]]*$$//')"; \
			[ -z "$$name" ] && continue; \
			img="$$imgs_dir/$$name.img"; \
			if [ ! -f "$$img" ]; then \
				echo "erro: disco não encontrado: $$img" >&2; \
				exit 1; \
			fi; \
			drive_flags+=(-drive "file=$$img,format=raw,if=ide,index=$$idx"); \
			attached+=("$$name"); \
			idx=$$((idx+1)); \
		done; \
	fi; \
	if [ "$${#attached[@]}" -eq 0 ]; then \
		echo "==> Nenhum disco anexado"; \
	else \
		echo "==> Discos anexados:$${attached[*]/#/ }"; \
	fi; \
	exec "$(QEMU)" $(QEMU_BASE_FLAGS) "$${drive_flags[@]}" $(QEMU_EXTRA_FLAGS)

# ============================================================
#  Apps
# ============================================================

# Uso: make app APP=helloworld
app:
	@if [ -z "$(APP)" ]; then \
		echo "uso: make app APP=<nome>"; exit 1; \
	fi
	@"$(MAKEAPPS)" "$(APP)"

# Compila todas as pastas em assets/Apps/Codes/
apps:
	@for entry in assets/Apps/Codes/*/; do \
		[ -d "$$entry" ] || continue; \
		name=$$(basename "$$entry"); \
		echo ">> app: $$name"; \
		"$(MAKEAPPS)" "$$name" || exit 1; \
	done

# ============================================================
#  Discos
# ============================================================

# Uso: make disk NAME=Disk0 SIZE=128 FS=FAT32 DIR=Disk0
disk:
	@if [ -z "$(NAME)" ] || [ -z "$(SIZE)" ] || [ -z "$(FS)" ] || [ -z "$(DIR)" ]; then \
		echo "uso: make disk NAME=<nome> SIZE=<MiB> FS=<FAT32|FAT16|FAT12|ExFAT> DIR=<pasta>"; \
		exit 1; \
	fi
	@"$(MKDISK)" "$(NAME)" "$(SIZE)" "$(FS)" "$(DIR)"

# Lê assets/Disks/Disks.json e reconstrói todos os discos (requer jq)
disks:
	@if ! command -v jq >/dev/null 2>&1; then \
		echo "erro: 'jq' não instalado (sudo apt install jq)"; exit 1; \
	fi
	@jq -c '.[]' assets/Disks/Disks.json | while read -r entry; do \
		name=$$(echo "$$entry" | jq -r '.name'); \
		size=$$(echo "$$entry" | jq -r '.size'); \
		fs=$$(echo   "$$entry" | jq -r '.fs');   \
		dir=$$(echo  "$$entry" | jq -r '.dir');  \
		echo ">> disk: $$name ($$size MiB, $$fs)"; \
		"$(MKDISK)" "$$name" "$$size" "$$fs" "$$dir" || exit 1; \
	done

# ============================================================
#  Limpeza
# ============================================================
clean:
	rm -rf "$(BUILD_DIR)"
	rm -rf "$(LOG_DIR)"

help:
	@echo "UpOS Makefile — targets:"
	@echo "  all                                abre o MakeGUI (default)"
	@echo "  system                             compila o kernel e gera o ISO"
	@echo "  kernel                             compila apenas o ELF do kernel"
	@echo "  run  [DISK=<img>]                  roda no QEMU (disco opcional)"
	@echo "  debug                              run + tail do log do QEMU"
	@echo "  app  APP=<nome>                    compila um app via makeapps.sh"
	@echo "  apps                               compila todos os apps"
	@echo "  disk NAME=.. SIZE=.. FS=.. DIR=..  cria imagem via mkdisk.sh"
	@echo "  disks                              reconstrói todos os discos do JSON"
	@echo "  clean                              apaga build/ e logs/"

-include $(ALL_DEPENDS)