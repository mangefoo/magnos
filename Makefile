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
HDD_IMG = hdd.img
HELLO_BIN = userspace/hello
PRINT_BIN = userspace/print
FILETEST_BIN = userspace/filetest
LS_BIN = userspace/ls

# Object files
KERNEL_ENTRY_OBJ = kernel_entry.o
KERNEL_OBJ = kernel.o
VGA_OBJ = vga.o
SERIAL_OBJ = serial.o
IDE_OBJ = ide.o
FAT32_OBJ = fat32.o
ELF_OBJ = elf.o
SYSCALL_OBJ = syscall.o
KEYBOARD_OBJ = keyboard.o

OBJS = $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(SERIAL_OBJ) $(IDE_OBJ) $(FAT32_OBJ) $(ELF_OBJ) $(SYSCALL_OBJ) $(KEYBOARD_OBJ)

# Default target
all: $(OS_IMG)

# Build bootloader
$(BOOT_BIN): boot.asm
	$(ASM) -f bin $< -o $@

# Build kernel entry
$(KERNEL_ENTRY_OBJ): kernel_entry.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Build kernel objects
$(KERNEL_OBJ): kernel.c vga.h serial.h ide.h fat32.h elf.h syscall.h keyboard.h
	$(CC) $(CFLAGS) -c $< -o $@

$(VGA_OBJ): vga.c vga.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERIAL_OBJ): serial.c serial.h
	$(CC) $(CFLAGS) -c $< -o $@

$(IDE_OBJ): ide.c ide.h
	$(CC) $(CFLAGS) -c $< -o $@

$(FAT32_OBJ): fat32.c fat32.h ide.h vga.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF_OBJ): elf.c elf.h vga.h syscall.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SYSCALL_OBJ): syscall.c syscall.h vga.h elf.h fat32.h serial.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KEYBOARD_OBJ): keyboard.c keyboard.h
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

# Build userspace hello program
$(HELLO_BIN): userspace/hello.c userspace/crt0.c userspace/libmagnos.h
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ userspace/crt0.c userspace/hello.c

# Build userspace print program
$(PRINT_BIN): userspace/print.c userspace/crt0.c userspace/libmagnos.h
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ userspace/crt0.c userspace/print.c

# Build userspace filetest program
$(FILETEST_BIN): userspace/filetest.c userspace/crt0.c userspace/libmagnos.h
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ userspace/crt0.c userspace/filetest.c

# Build userspace ls program
$(LS_BIN): userspace/ls.c userspace/crt0.c userspace/libmagnos.h
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ userspace/crt0.c userspace/ls.c

# Create hard disk image (10MB) formatted as FAT32
$(HDD_IMG): $(HELLO_BIN) $(PRINT_BIN) $(FILETEST_BIN) $(LS_BIN)
	dd if=/dev/zero of=$@ bs=1M count=10
	mkfs.fat -F 32 $@
	@echo "Created 10MB FAT32 disk image"
	@if [ -f hello.txt ]; then \
		mcopy -i $@ hello.txt ::HELLO.TXT && echo "Added hello.txt to disk"; \
	fi
	@if [ -f $(HELLO_BIN) ]; then \
		mcopy -i $@ $(HELLO_BIN) ::HELLO && echo "Added hello binary to disk"; \
	fi
	@if [ -f $(PRINT_BIN) ]; then \
		mcopy -i $@ $(PRINT_BIN) ::PRINT && echo "Added print binary to disk"; \
	fi
	@if [ -f $(FILETEST_BIN) ]; then \
		mcopy -i $@ $(FILETEST_BIN) ::FILETEST && echo "Added filetest binary to disk"; \
	fi
	@if [ -f $(LS_BIN) ]; then \
		mcopy -i $@ $(LS_BIN) ::LS && echo "Added ls binary to disk"; \
	fi

# Run in QEMU (no hard disk)
run: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -serial stdio

# Run in QEMU with hard disk
run-hdd: $(OS_IMG) $(HDD_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,if=floppy -drive file=$(HDD_IMG),format=raw,if=ide,index=0,media=disk -boot a -serial stdio

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
	rm -f *.o *.bin *.elf *.img serial.log userspace/hello userspace/print userspace/filetest userspace/ls userspace/*.o

.PHONY: all run run-hdd run-serial-file run-monitor debug clean
