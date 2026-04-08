# Makefile for MagnOS

# Cross-compiler prefix
# macOS/Apple Silicon: brew install i686-elf-gcc (default)
# Linux: override with `make CROSS=` to use native gcc -m32
CROSS ?= i686-elf-

# Tools
ASM = nasm
CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
MKFS_FAT = $(shell which mkfs.fat 2>/dev/null || echo /opt/homebrew/sbin/mkfs.fat)

# Directories
BOOT_DIR = bootloader
KERN_DIR = kernel
USER_DIR = userspace
BUILD_DIR = build

# Flags
ASMFLAGS = -f elf32
ifeq ($(CROSS),)
  ARCH_CFLAGS = -m32
  ARCH_LDFLAGS = -m elf_i386
else
  ARCH_CFLAGS =
  ARCH_LDFLAGS =
endif
CFLAGS = $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector -Wall -Wextra -I$(KERN_DIR)
LDFLAGS = $(ARCH_LDFLAGS) -T $(KERN_DIR)/linker.ld

# Output files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMG = magnos.img
HDD_IMG = hdd.img
HELLO_BIN = $(USER_DIR)/hello
PRINT_BIN = $(USER_DIR)/print
LS_BIN = $(USER_DIR)/ls
CAT_BIN = $(USER_DIR)/cat
SHELL_BIN = $(USER_DIR)/shell
UPTIME_BIN = $(USER_DIR)/uptime
COUNT_BIN = $(USER_DIR)/count
FREE_BIN = $(USER_DIR)/free
PFTEST_BIN = $(USER_DIR)/pftest
RING3_BIN = $(USER_DIR)/ring3

# Kernel object files
KERN_OBJS = \
	$(BUILD_DIR)/kernel_entry.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/vga.o \
	$(BUILD_DIR)/serial.o \
	$(BUILD_DIR)/ide.o \
	$(BUILD_DIR)/fat32.o \
	$(BUILD_DIR)/elf.o \
	$(BUILD_DIR)/syscall.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/args.o \
	$(BUILD_DIR)/setjmp.o \
	$(BUILD_DIR)/idt.o \
	$(BUILD_DIR)/isr.o \
	$(BUILD_DIR)/pmm.o \
	$(BUILD_DIR)/heap.o \
	$(BUILD_DIR)/paging.o \
	$(BUILD_DIR)/process.o \
	$(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/gdt_flush.o

# Default target
all: $(OS_IMG)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build bootloader
$(BOOT_BIN): $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

# Build kernel assembly objects
$(BUILD_DIR)/kernel_entry.o: $(KERN_DIR)/kernel_entry.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/setjmp.o: $(KERN_DIR)/setjmp.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/gdt_flush.o: $(KERN_DIR)/gdt_flush.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/isr.o: $(KERN_DIR)/isr.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Build kernel C objects
$(BUILD_DIR)/%.o: $(KERN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel to ELF
$(KERNEL_ELF): $(KERN_OBJS) $(KERN_DIR)/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERN_OBJS)

# Convert ELF to flat binary
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# Create OS image
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	# Pad to at least 1.44MB (floppy size)
	@SIZE=$$(stat -f%z $@ 2>/dev/null || stat -c%s $@); \
	if [ $$SIZE -lt 1474560 ]; then \
		dd if=/dev/zero bs=1 count=$$((1474560 - $$SIZE)) >> $@; \
	fi

# Build userspace programs
$(HELLO_BIN): $(USER_DIR)/hello.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/hello.c

$(PRINT_BIN): $(USER_DIR)/print.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/print.c

$(LS_BIN): $(USER_DIR)/ls.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/ls.c

$(CAT_BIN): $(USER_DIR)/cat.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/cat.c

$(SHELL_BIN): $(USER_DIR)/shell.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/shell.c

$(UPTIME_BIN): $(USER_DIR)/uptime.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/uptime.c

$(COUNT_BIN): $(USER_DIR)/count.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/count.c

$(FREE_BIN): $(USER_DIR)/free.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/free.c

$(PFTEST_BIN): $(USER_DIR)/pftest.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/pftest.c

$(RING3_BIN): $(USER_DIR)/ring3.c $(USER_DIR)/crt0.c $(USER_DIR)/libmagnos.h
	$(CC) $(ARCH_CFLAGS) -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
		-static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
		-o $@ $(USER_DIR)/crt0.c $(USER_DIR)/ring3.c

# Create hard disk image (10MB) formatted as FAT32
$(HDD_IMG): $(HELLO_BIN) $(PRINT_BIN) $(LS_BIN) $(CAT_BIN) $(SHELL_BIN) $(UPTIME_BIN) $(COUNT_BIN) $(FREE_BIN) $(PFTEST_BIN) $(RING3_BIN)
	dd if=/dev/zero of=$@ bs=1M count=10
	$(MKFS_FAT) -F 32 $@
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
	@if [ -f $(LS_BIN) ]; then \
		mcopy -i $@ $(LS_BIN) ::LS && echo "Added ls binary to disk"; \
	fi
	@if [ -f $(CAT_BIN) ]; then \
		mcopy -i $@ $(CAT_BIN) ::CAT && echo "Added cat binary to disk"; \
	fi
	@if [ -f $(SHELL_BIN) ]; then \
		mcopy -i $@ $(SHELL_BIN) ::SHELL && echo "Added shell binary to disk"; \
	fi
	@if [ -f $(UPTIME_BIN) ]; then \
		mcopy -i $@ $(UPTIME_BIN) ::UPTIME && echo "Added uptime binary to disk"; \
	fi
	@if [ -f $(COUNT_BIN) ]; then \
		mcopy -i $@ $(COUNT_BIN) ::COUNT && echo "Added count binary to disk"; \
	fi
	@if [ -f $(FREE_BIN) ]; then \
		mcopy -i $@ $(FREE_BIN) ::FREE && echo "Added free binary to disk"; \
	fi
	@if [ -f $(PFTEST_BIN) ]; then \
		mcopy -i $@ $(PFTEST_BIN) ::PFTEST && echo "Added pftest binary to disk"; \
	fi
	@if [ -f $(RING3_BIN) ]; then \
		mcopy -i $@ $(RING3_BIN) ::RING3 && echo "Added ring3 binary to disk"; \
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
	rm -rf $(BUILD_DIR) *.img serial.log $(USER_DIR)/hello $(USER_DIR)/print $(USER_DIR)/ls $(USER_DIR)/cat $(USER_DIR)/shell $(USER_DIR)/uptime $(USER_DIR)/count $(USER_DIR)/free $(USER_DIR)/pftest $(USER_DIR)/ring3 $(USER_DIR)/*.o

.PHONY: all run run-hdd run-serial-file run-monitor debug clean
