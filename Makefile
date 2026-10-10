# ============================================================
#  UpOS Makefile — space-safe
#  Targets/prereqs: unquoted (relative, no spaces)
#  Recipes: every path quoted
# ============================================================

# ---- Tools ----
ASM      = nasm
CC       = gcc
CXX      = g++
LD       = ld
GRUB     = grub-mkrescue
QEMU     = qemu-system-i386
MCOPY    = mcopy
DD       = dd
MKFSFAT  = mkfs.vfat

# ---- Flags (kernel) ----
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -nostdlib -fno-builtin \
           -fno-stack-protector -fno-pic -Wall -Wextra \
           -Ikernel -Ikernel/drivers -g -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  = -m elf_i386 -T kernel/linker.ld -O2

# ---- Flags (apps) ----
APP_CFLAGS   = -m32 -ffreestanding -nostdlib -fno-builtin \
               -fno-stack-protector -fno-pic -Wall -O2
APP_CXXFLAGS = $(APP_CFLAGS) -fno-exceptions -fno-rtti

# ---- Absolute paths (may contain spaces; ONLY used inside recipes) ----
PROJECT_ROOT := $(CURDIR)
ASSETS_DIR   := $(PROJECT_ROOT)/assets

APPS_DIR       := $(ASSETS_DIR)/Apps
APPS_CODES_DIR := $(APPS_DIR)/Codes
APPS_BIN_DIR   := $(APPS_DIR)/Binaries

DISKS_DIR      := $(ASSETS_DIR)/Disks
DISKS_DATA_DIR := $(DISKS_DIR)/Data
DISKS_IMGS_DIR := $(DISKS_DIR)/Images

# ---- Relative build dirs (no spaces → safe as targets) ----
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

# ---- Final files (relative → safe as targets) ----
KERNEL_ELF   = $(BUILD_DIR)/UpKnel.elf
ISO_FILE     = $(BUILD_DIR)/UpOS.iso
GRUB_CFG_SRC = $(KERNEL_DIR)/grub.cfg
GRUB_CFG_DST = $(GRUB_DIR)/grub.cfg

# ---- Default disk ----
DISK_IMG      := $(DISKS_IMGS_DIR)/UpOS.img
DISK_SIZE_MB  = 64
DISK_LABEL    = UPOS
DISK_DATA_SRC := $(DISKS_DATA_DIR)/Boot

# ---- GUI parameters ----
DISK_NAME ?=
DISK_SIZE ?= 64
DISK_FS   ?= FAT32
DISK_DIR  ?=
DISK_DATA ?= $(DISKS_DATA_DIR)
DISK_OUT  ?= $(DISKS_IMGS_DIR)/$(DISK_NAME).img

APP_FILE      ?=
APP_CODES     ?= $(APPS_CODES_DIR)
APP_OUT_DIR   ?= $(APPS_BIN_DIR)
APP_SRC       = $(APP_CODES)/$(APP_FILE)
APP_STEM      = $(basename $(notdir $(APP_FILE)))
APP_OUT       = $(APP_OUT_DIR)/$(APP_STEM).bin
APP_BUILD_DIR = $(BUILD_DIR)/apps/$(APP_STEM)

# ---- Kernel sources ----
C_SOURCES    := $(shell find $(KERNEL_DIR) -name '*.c' 2>/dev/null)
CXX_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.cpp' -o -name '*.c++' 2>/dev/null)
ASM_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.asm' 2>/dev/null)
GAS_SOURCES  := $(shell find $(KERNEL_DIR) -name '*.s'  2>/dev/null)

C_OBJECTS    := $(patsubst $(KERNEL_DIR)/%.c,   $(OBJ_DIR)/%.o, $(C_SOURCES))
CXX_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CXX_SOURCES))
CXX_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.c++, $(OBJ_DIR)/%.o, $(CXX_OBJECTS))
ASM_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.asm, $(OBJ_DIR)/%.o, $(ASM_SOURCES))
GAS_OBJECTS  := $(patsubst $(KERNEL_DIR)/%.s,   $(OBJ_DIR)/%.o, $(GAS_SOURCES))
ALL_OBJECTS  := $(C_OBJECTS) $(CXX_OBJECTS) $(ASM_OBJECTS) $(GAS_OBJECTS)

C_DEPENDS    := $(patsubst $(KERNEL_DIR)/%.c,   $(DEP_DIR)/%.d, $(C_SOURCES))
CXX_DEPENDS  := $(patsubst $(KERNEL_DIR)/%.cpp, $(DEP_DIR)/%.d, $(CXX_SOURCES))
CXX_DEPENDS  := $(patsubst $(KERNEL_DIR)/%.c++, $(DEP_DIR)/%.d, $(CXX_DEPENDS))
ALL_DEPENDS  := $(C_DEPENDS) $(CXX_DEPENDS)

# ============================================================
.PHONY: all system cls kernel run debug clean help \
        disk disk_ensure disk_format disk_update disk_delete app

all:
	@~/UpOS/.venv/bin/python3 assets/Tools/Codes/MakeGUI/main.py

system: $(ISO_FILE)

cls:
	clear

kernel: $(KERNEL_ELF)

# ---------- targets/prereqs SEM aspas ----------
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
#  DISK
# ============================================================

disk_ensure:
	@if [ ! -f "$(DISK_IMG)" ]; then \
		"$(MAKE)" --no-print-directory disk_format; \
	fi

disk_format:
	@echo "==> Formatting default disk: $(DISK_IMG) ($(DISK_SIZE_MB) MB, label $(DISK_LABEL))"
	@mkdir -p "$(DISKS_IMGS_DIR)"
	@rm -f "$(DISK_IMG)"
	@"$(DD)" if=/dev/zero of="$(DISK_IMG)" bs=1M count=$(DISK_SIZE_MB) status=none
	@"$(MKFSFAT)" -F 32 -n "$(DISK_LABEL)" "$(DISK_IMG)" > /dev/null
	@echo "==> Default disk ready."

disk_update: disk_ensure
	@echo "==> Syncing $(DISK_DATA_SRC) <-> $(DISK_IMG)"
	@mkdir -p "$(DISK_DATA_SRC)"
	@if [ -n "$$(ls -A "$(DISK_DATA_SRC)" 2>/dev/null)" ]; then \
		"$(MCOPY)" -o -i "$(DISK_IMG)" -s "$(DISK_DATA_SRC)"/* ::/ ; \
	else \
		echo "        (source empty, nothing to upload)"; \
	fi
	@"$(MCOPY)" -n -o -i "$(DISK_IMG)" ::/ "$(DISK_DATA_SRC)"
	@echo "==> Sync done."

disk_delete:
	@echo "==> Removing $(DISK_IMG)"
	@rm -f "$(DISK_IMG)"
	@echo "==> Done."

disk:
	@if [ -z "$(DISK_NAME)" ]; then \
		echo "ERROR: DISK_NAME is required."; exit 1; \
	fi
	@if [ -z "$(DISK_DIR)" ]; then \
		echo "ERROR: DISK_DIR is required."; exit 1; \
	fi
	@echo "==> Building disk '$(DISK_NAME)'"
	@echo "    size   = $(DISK_SIZE) MB"
	@echo "    fs     = $(DISK_FS)"
	@echo "    data   = $(DISK_DATA)/$(DISK_DIR)/"
	@echo "    output = $(DISK_OUT)"
	@mkdir -p "$(DISK_DATA)/$(DISK_DIR)" "$(dir $(DISK_OUT))"
	@rm -f "$(DISK_OUT)"
	@"$(DD)" if=/dev/zero of="$(DISK_OUT)" bs=1M count=$(DISK_SIZE) status=none
	@case "$(DISK_FS)" in \
		FAT32|fat32) "$(MKFSFAT)" -F 32 -n "$(DISK_NAME)" "$(DISK_OUT)" > /dev/null ;; \
		FAT16|fat16) "$(MKFSFAT)" -F 16 -n "$(DISK_NAME)" "$(DISK_OUT)" > /dev/null ;; \
		FAT12|fat12) "$(MKFSFAT)" -F 12 -n "$(DISK_NAME)" "$(DISK_OUT)" > /dev/null ;; \
		*) echo "ERROR: unsupported FS '$(DISK_FS)'"; exit 1 ;; \
	esac
	@if [ -n "$$(ls -A "$(DISK_DATA)/$(DISK_DIR)" 2>/dev/null)" ]; then \
		"$(MCOPY)" -o -i "$(DISK_OUT)" -s "$(DISK_DATA)/$(DISK_DIR)"/* ::/ ; \
	else \
		echo "        (source empty, nothing to copy)"; \
	fi
	@echo "==> Disk '$(DISK_NAME)' built at $(DISK_OUT)"

# ============================================================
#  APP
# ============================================================

app:
	@if [ -z "$(APP_FILE)" ]; then \
		echo "ERROR: APP_FILE is required."; exit 1; \
	fi
	@if [ ! -e "$(APP_SRC)" ]; then \
		echo "ERROR: source not found: $(APP_SRC)"; exit 1; \
	fi
	@mkdir -p "$(APP_BUILD_DIR)" "$(APP_OUT_DIR)"
	@if [ -d "$(APP_SRC)" ]; then \
		SOURCES=$$(find "$(APP_SRC)" -type f \
			\( -name '*.c' -o -name '*.cpp' -o -name '*.c++' \
			   -o -name '*.asm' -o -name '*.s' \) | sort); \
		KIND="folder"; \
	else \
		SOURCES="$(APP_SRC)"; \
		KIND="file"; \
	fi; \
	if [ -z "$$SOURCES" ]; then \
		echo "ERROR: no source files found in $(APP_SRC)"; \
		rm -rf "$(APP_BUILD_DIR)"; exit 1; \
	fi; \
	echo "==> Building app '$(APP_FILE)' ($$KIND)"; \
	echo "    output = $(APP_OUT)"; \
	OBJS=""; \
	for src in $$SOURCES; do \
		obj="$(APP_BUILD_DIR)/$$(echo "$$src" | tr '/' '_').o"; \
		echo "    [cc] $$src"; \
		case "$$src" in \
			*.c) \
				"$(CC)" $(APP_CFLAGS) -c "$$src" -o "$$obj" || exit 1 ;; \
			*.cpp|*.c++) \
				"$(CXX)" $(APP_CXXFLAGS) -c "$$src" -o "$$obj" || exit 1 ;; \
			*.asm) \
				"$(ASM)" -f elf32 "$$src" -o "$$obj" || exit 1 ;; \
			*.s) \
				"$(CC)" $(APP_CFLAGS) -c "$$src" -o "$$obj" || exit 1 ;; \
		esac; \
		OBJS="$$OBJS $$obj"; \
	done; \
	echo "    [ld] linking $$(echo $$OBJS | wc -w) object(s)"; \
	"$(LD)" -m elf_i386 -Ttext 0x0 --oformat binary $$OBJS -o "$(APP_OUT)" || \
		(rm -rf "$(APP_BUILD_DIR)"; exit 1); \
	rm -rf "$(APP_BUILD_DIR)"; \
	echo "==> App '$(APP_FILE)' built."

# ============================================================
#  QEMU
# ============================================================

QEMU_EXTRA_FLAGS :=

run: $(ISO_FILE) disk_ensure
	@mkdir -p "$(LOG_DIR)"
	@echo "==> QEMU log: $(QEMU_LOG)"
	"$(QEMU)" -cdrom "$(ISO_FILE)" -boot d \
		-display gtk \
		-drive file="$(DISK_IMG)",format=raw,if=ide,index=0 \
		-rtc base=localtime \
		-no-reboot -no-shutdown \
		-vga std \
		-cpu max \
		-d int,cpu_reset \
		-D "$(QEMU_LOG)" \
		-serial "file:$(LOG_DIR)/serial.log" \
		-enable-kvm \
		$(QEMU_EXTRA_FLAGS)

debug: run
	@echo ""
	@echo "==== Last 80 lines of QEMU log ===="
	@tail -n 80 "$(QEMU_LOG)" 2>/dev/null || true
	@echo "==================================="

clean:
	rm -rf "$(BUILD_DIR)"
	rm -rf "$(LOG_DIR)"

help:
	@echo "UpOS Makefile — targets:"
	@echo "  system, run, debug, clean, disk, disk_format, disk_update, disk_delete, app"

-include $(ALL_DEPENDS)