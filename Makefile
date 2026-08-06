CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -Wall -Wextra -I./system/libs -I./system/drivers -I./system/interrupt
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T system/kernel/link.ld

OBJ_DIR = build
KERNEL_ELF = $(OBJ_DIR)/UpdateOS.elf
KERNEL_BIN = $(OBJ_DIR)/UpdateOS.bin
ISO_DIR = $(OBJ_DIR)/iso
GRUB_CFG = system/kernel/grub.cfg
ISO_FILE = $(OBJ_DIR)/UpdateOS.iso

# Exclui arquivos de exemplo ou duplicados (keyboard_irq, kernel_example, isr_handlers)
C_SRCS = $(shell find system -name '*.c' ! -name 'keyboard_irq.c' ! -name 'kernel_example.c')
ASM_SRCS = $(shell find system -name '*.asm' ! -name 'isr_handlers.asm' ! -name 'irq_handlers.asm')
# Incluímos apenas o irq_handlers.asm (que é o correto)
ASM_SRCS += system/interrupt/irq_handlers.asm

OBJS = $(patsubst system/%.c, $(OBJ_DIR)/%.o, $(C_SRCS)) \
       $(patsubst system/%.asm, $(OBJ_DIR)/%.o, $(ASM_SRCS))

.PHONY: all clean run

all: run

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: system/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: system/%.asm | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	cp $< $@

$(ISO_DIR)/boot/grub:
	mkdir -p $@

$(ISO_DIR)/boot/UpdateOS.bin: $(KERNEL_BIN) | $(ISO_DIR)/boot/grub
	cp $< $@

$(ISO_DIR)/boot/grub/grub.cfg: $(GRUB_CFG) | $(ISO_DIR)/boot/grub
	cp $< $@

iso: $(ISO_DIR)/boot/UpdateOS.bin $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)

run: iso
	cmd.exe /c "qemu-system-x86_64 -cdrom $(ISO_FILE)"

clean:
	rm -rf $(OBJ_DIR)
