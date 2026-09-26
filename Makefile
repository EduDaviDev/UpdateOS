# ============================================================
# Makefile do UpdateOS
# ============================================================

# ---- Ferramentas ----
ASM      = nasm
CC       = gcc
CXX      = g++
LD       = ld
GRUB     = grub-mkrescue
QEMU     = qemu-system-i386
DD       = dd
MKFSFAT  = mkfs.vfat

# ---- Flags ----
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -nostdlib -fno-builtin \
           -fno-stack-protector -fno-pic -Wall -Wextra \
           -Ikernel -Ikernel/drivers -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  = -m elf_i386 -T kernel/linker.ld

# ---- Diretórios ----
KERNEL_DIR   = kernel
BUILD_DIR    = build
OBJ_DIR      = $(BUILD_DIR)/kernel
DEP_DIR      = $(BUILD_DIR)/dependencies
ISO_DIR      = $(BUILD_DIR)/iso
BOOT_DIR     = $(ISO_DIR)/boot
GRUB_DIR     = $(BOOT_DIR)/grub
SYSTEM_DIR   = $(ISO_DIR)/system
LOG_DIR      = logs
QEMU_LOG     = $(LOG_DIR)/qemu.log

# ---- Arquivos finais ----
KERNEL_ELF   = $(BUILD_DIR)/UpKnel.elf
ISO_FILE     = $(BUILD_DIR)/UpOS.iso
GRUB_CFG_SRC = $(KERNEL_DIR)/grub.cfg
GRUB_CFG_DST = $(GRUB_DIR)/grub.cfg

# ---- Disco FAT32 (raiz do sistema de arquivos) ----
DISK_IMG     = UpOS.img
DISK_SIZE_MB = 512
DISK_LABEL   = UPOS

# ---- Fontes ----
C_SOURCES    := $(shell find $(KERNEL_DIR) -name '*.c')
CXX_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.cpp' -o -name '*.c++')
ASM_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.asm')
GAS_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.s')

# ---- Objetos ----
C_OBJECTS    := $(patsubst $(KERNEL_DIR)/%.c,   $(OBJ_DIR)/%.o, $(C_SOURCES))
CXX_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CXX_SOURCES))
CXX_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.c++, $(OBJ_DIR)/%.o, $(CXX_OBJECTS))
ASM_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.asm, $(OBJ_DIR)/%.o, $(ASM_SOURCES))
GAS_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.s,   $(OBJ_DIR)/%.o, $(GAS_SOURCES))

ALL_OBJECTS  := $(C_OBJECTS) $(CXX_OBJECTS) $(ASM_OBJECTS) $(GAS_OBJECTS)

# ---- Dependências ----
C_DEPENDS    := $(patsubst $(KERNEL_DIR)/%.c,   $(DEP_DIR)/%.d, $(C_SOURCES))
CXX_DEPENDS  := $(patsubst $(KERNEL_DIR)/%.cpp, $(DEP_DIR)/%.d, $(CXX_SOURCES))
CXX_DEPENDS  := $(patsubst $(KERNEL_DIR)/%.c++, $(DEP_DIR)/%.d, $(CXX_DEPENDS))
ALL_DEPENDS  := $(C_DEPENDS) $(CXX_DEPENDS)

# ============================================================
# Alvos principais
# ============================================================

.PHONY: all clean run iso kernel debug disk disk-clean

all: $(ISO_FILE) $(DISK_IMG)

# ---- Linkagem do kernel ----
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(ALL_OBJECTS) $(KERNEL_DIR)/linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)

# ---- Compilação C ----
$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@) $(DEP_DIR)/$(dir $*)
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# ---- Compilação C++ ----
$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.cpp
	@mkdir -p $(dir $@) $(DEP_DIR)/$(dir $*)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.c++
	@mkdir -p $(dir $@) $(DEP_DIR)/$(dir $*)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# ---- Compilação NASM ----
$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# ---- Compilação GAS ----
$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Criação da ISO ----
$(ISO_FILE): $(KERNEL_ELF) $(GRUB_CFG_SRC)
	@mkdir -p $(BOOT_DIR) $(GRUB_DIR) $(SYSTEM_DIR)
	cp $(KERNEL_ELF) $(BOOT_DIR)/upkernel.elf
	cp $(GRUB_CFG_SRC) $(GRUB_CFG_DST)
	@if [ -d "$(KERNEL_DIR)/iso" ]; then \
		cp -r $(KERNEL_DIR)/iso/* $(SYSTEM_DIR)/ 2>/dev/null || true; \
	fi
	$(GRUB) -o $(ISO_FILE) $(ISO_DIR)

# ---- Criação do disco FAT32 (a raiz do FS é a raiz do disco) ----
$(DISK_IMG):
	@echo "==> Criando disco FAT32 de $(DISK_SIZE_MB) MB: $@"
	@$(DD) if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE_MB) status=none 2>/dev/null || \
		$(DD) if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE_MB)
	@$(MKFSFAT) -F 32 -n $(DISK_LABEL) $@
	@echo "==> Disco pronto. Raiz FAT32 vazia, pronta para uso."

disk: $(DISK_IMG)

disk-clean:
	@echo "==> Removendo disco (todos os dados serao perdidos)"
	rm -f $(DISK_IMG)

# ---- Executar no QEMU ----
run: $(ISO_FILE) $(DISK_IMG)
	@mkdir -p $(LOG_DIR)
	@echo "==> Log do QEMU sera salvo em $(QEMU_LOG)"
	cmd.exe /c "if exist $(QEMU_LOG) del /q $(QEMU_LOG)" 2>/dev/null || true
	cmd.exe /c "$(QEMU) -cdrom $(ISO_FILE) -boot d \
		-drive file=$(DISK_IMG),format=raw,if=ide,index=0 \
		-rtc base=localtime \
		-no-reboot -no-shutdown \
		-d int,cpu_reset \
		-D $(QEMU_LOG) \
		-serial file:$(LOG_DIR)/serial.log"

debug: run
	@echo ""
	@echo "==== Ultimas 80 linhas do log do QEMU ===="
	@tail -n 80 $(QEMU_LOG) 2>/dev/null || type $(QEMU_LOG)
	@echo "=========================================="

-include $(ALL_DEPENDS)
# ---- Limpeza ----
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LOG_DIR)