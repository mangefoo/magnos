# MagnOS

A bootable x86 operating system written in C and assembly, built from scratch as a learning project.

## Features

- Custom bootloader with protected mode switching
- VGA text mode driver with color support and hardware cursor
- Serial port (COM1) driver
- PS/2 keyboard driver (interrupt-driven via IRQ1)
- IDE/ATA hard disk driver
- FAT32 filesystem (read-only)
- ELF binary loader with nested execution support
- Interrupt Descriptor Table (IDT) with exception handlers
- PIC remapping and PIT timer (100 Hz tick)
- Userspace shell with built-in commands (`clear`, `exit`)
- Syscall interface for userspace programs

## Requirements

### macOS (Apple Silicon)

```bash
brew install i686-elf-gcc nasm qemu mtools dosfstools
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install nasm gcc-multilib qemu-system-x86 mtools dosfstools
```

## Building

```bash
make              # Build the OS image (magnos.img)
make CROSS=       # On Linux with native gcc (uses -m32)
```

## Running

```bash
make run          # Boot in QEMU (floppy only, falls back to kernel shell)
make run-hdd      # Boot with FAT32 hard disk (launches userspace shell)
make debug        # Boot with GDB server on port 1234
```

The OS outputs to both the QEMU VGA window and the terminal (serial). Type in either.

## Project Structure

```
magnos/
├── bootloader/
│   └── boot.asm          # Bootloader: disk load, GDT, protected mode switch
├── kernel/
│   ├── kernel.c           # Main kernel: init, shell fallback
│   ├── kernel_entry.asm   # Entry point: stack setup, calls kernel_main
│   ├── vga.c/h            # VGA text mode driver
│   ├── serial.c/h         # Serial port (COM1) driver
│   ├── keyboard.c/h       # PS/2 keyboard (interrupt-driven, ring buffer)
│   ├── ide.c/h            # IDE/ATA disk driver
│   ├── fat32.c/h          # FAT32 filesystem
│   ├── elf.c/h            # ELF binary loader
│   ├── syscall.c/h        # Syscall handler (10 syscalls)
│   ├── idt.c/h            # IDT, PIC remap, PIT timer, interrupt dispatcher
│   ├── isr.asm            # ISR stubs (exceptions 0-31, IRQs 32-47)
│   ├── setjmp.asm         # setjmp/longjmp for nested exec return
│   ├── args.c/h           # Command-line argument parsing
│   ├── io.h               # Shared port I/O (inb, outb)
│   └── linker.ld          # Kernel memory layout (base 0x1000)
├── userspace/
│   ├── crt0.c             # C runtime startup
│   ├── libmagnos.h        # Syscall wrappers for userspace
│   ├── shell.c            # Interactive shell
│   ├── ls.c               # Directory listing
│   ├── cat.c              # File display
│   ├── hello.c            # Hello world
│   └── print.c            # Print utility
├── build/                  # Build artifacts (generated)
├── Makefile
└── hello.txt               # Sample text file for the FAT32 disk
```

## Memory Layout

```
0x1000 - 0x4xxx    .text     Kernel code
0x4xxx - 0x4xxx    .rodata   Strings, constants
0x4xxx - 0x12xxxx  .bss      Static buffers
0x1F0000           Stack     Kernel stack (grows downward)
0x200000           Userspace Program load address
0xB8000            VGA       Text mode buffer (80x25)
```

## Boot Process

1. BIOS loads bootloader (512 bytes) at 0x7C00
2. Bootloader loads kernel from disk to 0x1000 (53 sectors across track boundaries)
3. Bootloader sets up GDT and switches to 32-bit protected mode
4. Kernel entry sets up stack at 0x1F0000, calls `kernel_main()`
5. Kernel initializes drivers (VGA, serial, keyboard, IDE, FAT32)
6. Kernel initializes IDT, remaps PIC, starts PIT timer, enables interrupts
7. Kernel launches userspace shell (falls back to kernel shell if unavailable)

## Adding Files to the Disk

```bash
mcopy -i hdd.img myfile.txt ::MYFILE.TXT
```

FAT32 uses uppercase 8.3 filenames.

## License

Educational code. Use freely.
