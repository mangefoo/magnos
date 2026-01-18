# MagnOS

A simple bootable x86 operating system written in C and assembly.

## Features

- Custom bootloader with protected mode switching
- VGA text mode driver with color support
- Serial port (COM1) driver for I/O
- Interactive serial console

## Requirements

To build and run MagnOS, you need:

- `nasm` (Netwide Assembler)
- `gcc` (GNU C Compiler) with 32-bit support
- `ld` (GNU Linker)
- `qemu-system-i386` (QEMU x86 emulator)

### Installing dependencies

On Ubuntu/Debian:
```bash
sudo apt-get install nasm gcc-multilib qemu-system-x86
```

On Fedora/RHEL:
```bash
sudo dnf install nasm gcc qemu-system-x86
```

On Arch Linux:
```bash
sudo pacman -S nasm gcc qemu-system-x86
```

## Building

To build the OS image:

```bash
make
```

This will create `magnos.img`, a bootable floppy disk image.

## Running

### Run in QEMU (recommended)

```bash
make run
```

This will start QEMU with the OS. You'll see output in both the QEMU window (VGA) and your terminal (serial).

### Run with QEMU monitor

```bash
make run-monitor
```

This starts QEMU with a monitor accessible via telnet on port 55555.

### Debug mode

```bash
make debug
```

This starts QEMU in debug mode, waiting for a GDB connection on port 1234.

To connect with GDB:
```bash
gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.bin"
```

## Architecture

### Files

- `boot.asm` - Bootloader (512 bytes, loads kernel and switches to protected mode)
- `kernel_entry.asm` - Kernel entry point (calls C kernel)
- `kernel.c` - Main kernel code
- `vga.c/vga.h` - VGA text mode driver
- `serial.c/serial.h` - Serial port driver (COM1)
- `linker.ld` - Linker script
- `Makefile` - Build system

### Memory Layout

- `0x7c00` - Bootloader loads here
- `0x1000` - Kernel loads here
- `0xB8000` - VGA text mode buffer

### Boot Process

1. BIOS loads bootloader at 0x7c00
2. Bootloader loads kernel from disk to 0x1000
3. Bootloader sets up GDT and switches to 32-bit protected mode
4. Bootloader jumps to kernel entry point
5. Kernel initializes VGA and serial drivers
6. Kernel displays welcome message and enters interactive loop

## Interacting with the OS

The OS provides an interactive echo loop via the serial port. Type in the terminal (where you ran `make run`) and see your input echoed back both in the terminal and on the VGA display.

## Cleaning

To remove all build artifacts:

```bash
make clean
```

## Extending

This is a minimal OS kernel. You can extend it by adding:

- Keyboard driver (PS/2)
- Interrupt handling (IDT)
- Memory management
- Process/task management
- File system support
- More device drivers

## License

This is educational code. Use freely.
