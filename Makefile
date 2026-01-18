# Makefile for MagnOS

# Tools
ASM = nasm
CC = gcc
LD = ld

# Flags
ASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

# Output files
BOOT_BIN = boot.bin
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
OS_IMG = magnos.img

# Object files
KERNEL_ENTRY_OBJ = kernel_entry.o
KERNEL_OBJ = kernel.o
VGA_OBJ = vga.o
SERIAL_OBJ = serial.o

OBJS = $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(SERIAL_OBJ)

# Default target
all: $(OS_IMG)

# Build bootloader
$(BOOT_BIN): boot.asm
	$(ASM) -f bin $< -o $@

# Build kernel entry
$(KERNEL_ENTRY_OBJ): kernel_entry.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Build kernel objects
$(KERNEL_OBJ): kernel.c vga.h serial.h
	$(CC) $(CFLAGS) -c $< -o $@

$(VGA_OBJ): vga.c vga.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERIAL_OBJ): serial.c serial.h
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel to ELF
$(KERNEL_ELF): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Convert ELF to flat binary
$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $< $@

# Create OS image
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	# Pad to at least 1.44MB (floppy size)
	@SIZE=$$(stat -c%s $@); \
	if [ $$SIZE -lt 1474560 ]; then \
		dd if=/dev/zero bs=1 count=$$((1474560 - $$SIZE)) >> $@; \
	fi

# Run in QEMU
run: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -serial stdio

# Run with serial output to file
run-serial-file: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -serial file:serial.log
	@echo "Serial output saved to serial.log"

# Run in QEMU with monitor
run-monitor: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -serial stdio -monitor telnet:127.0.0.1:55555,server,nowait

# Debug with QEMU
debug: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -serial stdio -s -S

# Clean build artifacts
clean:
	rm -f *.o *.bin *.elf *.img serial.log

.PHONY: all run run-serial-file run-monitor debug clean
