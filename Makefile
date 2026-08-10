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

# Imagem FAT32
FAT32_IMG = $(OBJ_DIR)/fat32.img
FAT32_SIZE = 64
FAT32_CONTENT = disk_contents/fat32

# Exclui arquivos de exemplo ou duplicados
C_SRCS = $(shell find system -name '*.c' ! -name 'keyboard_irq.c' ! -name 'kernel_example.c')
ASM_SRCS = $(shell find system -name '*.asm' ! -name 'isr_handlers.asm' ! -name 'irq_handlers.asm')
ASM_SRCS += system/interrupt/irq_handlers.asm

OBJS = $(patsubst system/%.c, $(OBJ_DIR)/%.o, $(C_SRCS)) \
       $(patsubst system/%.asm, $(OBJ_DIR)/%.o, $(ASM_SRCS))

.PHONY: all clean run disk

all: iso disk run

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

# A regra disk agora depende da imagem FAT32
disk: $(FAT32_IMG)

$(FAT32_IMG): | $(OBJ_DIR)
	@echo "Criando imagem FAT32 de $(FAT32_SIZE) MiB..."
	dd if=/dev/zero of=$@ bs=1M count=$(FAT32_SIZE) status=progress
	mkfs.fat -F 32 -I $@

	# Cria o diretório de conteúdo se não existir
	@mkdir -p "$(CURDIR)/$(FAT32_CONTENT)"

	# Se o diretório estiver vazio, cria um arquivo de exemplo
	@if [ -z "$$(find "$(CURDIR)/$(FAT32_CONTENT)" -maxdepth 1 -type f 2>/dev/null)" ]; then \
		echo "Criando arquivo de exemplo 'teste.txt'..."; \
		echo "Hello, FAT32!" > "$(CURDIR)/$(FAT32_CONTENT)/teste.txt"; \
	fi

	# Copia todos os arquivos (recursivamente) para a raiz da imagem
	@echo "Copiando arquivos para a imagem..."
	@if [ -n "$$(ls -A "$(CURDIR)/$(FAT32_CONTENT)" 2>/dev/null)" ]; then \
		mcopy -s -i $@ "$(CURDIR)/$(FAT32_CONTENT)"/* ::/ ; \
	else \
		echo "Nenhum arquivo para copiar."; \
	fi

run: iso disk
	cmd.exe /c "qemu-system-x86_64 -serial stdio -cdrom $(ISO_FILE) -boot d -drive file=$(FAT32_IMG),format=raw"

clean:
	rm -rf $(OBJ_DIR)