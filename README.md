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
- Interrupt Descriptor Table (IDT) with exception handlers and page fault diagnostics
- PIC remapping and PIT timer (100 Hz tick)
- Physical memory manager (PMM) — bitmap-based page allocator (4KB pages)
- Kernel heap — `kmalloc()`/`kfree()` with free-list allocator
- Paging — identity-mapped virtual memory (0–16MB)
- GDT with ring 0/ring 3 segments and Task State Segment (TSS)
- Ring 3 userspace — programs run in user mode with kernel memory protection
- Syscall interface via `int $0x80` (14 syscalls)
- Userspace shell with built-in commands (`clear`, `exit`)

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
│   └── boot.asm           # Bootloader: disk load, GDT, protected mode switch
├── kernel/
│   ├── kernel.c           # Main kernel: init sequence, shell fallback
│   ├── kernel_entry.asm   # Entry point: stack setup, calls kernel_main
│   ├── vga.c/h            # VGA text mode driver
│   ├── serial.c/h         # Serial port (COM1) driver
│   ├── keyboard.c/h       # PS/2 keyboard (interrupt-driven, ring buffer)
│   ├── ide.c/h            # IDE/ATA disk driver
│   ├── fat32.c/h          # FAT32 filesystem
│   ├── elf.c/h            # ELF binary loader (ring 3 transition via iret)
│   ├── syscall.c/h        # Syscall handler (14 syscalls via int 0x80)
│   ├── idt.c/h            # IDT, PIC, PIT timer, interrupt dispatcher
│   ├── isr.asm            # ISR stubs (exceptions 0-31, IRQs 32-47, syscall 128)
│   ├── gdt.c/h            # GDT with kernel/user segments and TSS
│   ├── gdt_flush.asm      # GDT/TSS loading (lgdt, ltr)
│   ├── pmm.c/h            # Physical memory manager (bitmap page allocator)
│   ├── heap.c/h           # Kernel heap (kmalloc/kfree)
│   ├── paging.c/h         # Virtual memory (identity-mapped 0-16MB)
│   ├── setjmp.asm         # setjmp/longjmp for nested exec return
│   ├── args.c/h           # Command-line argument parsing
│   ├── io.h               # Port I/O (inb, outb)
│   └── linker.ld          # Kernel memory layout (base 0x1000)
├── userspace/
│   ├── crt0.c             # C runtime startup
│   ├── libmagnos.h        # Syscall wrappers (int 0x80)
│   ├── shell.c            # Interactive shell
│   ├── ls.c               # Directory listing
│   ├── cat.c              # File display
│   ├── hello.c            # Hello world
│   ├── print.c            # Print with arguments
│   ├── uptime.c           # System uptime
│   ├── count.c            # Count 1-5 with 1s delay (demonstrates sleep)
│   ├── free.c             # Memory statistics (PMM + heap)
│   ├── ring3.c            # Ring 3 protection demo
│   └── pftest.c           # Page fault test
├── Makefile
└── hello.txt              # Sample text file for the FAT32 disk
```

## Memory Layout

```
0x00000000 - 0x00000FFF   Real mode IVT/BIOS data
0x00001000 - 0x00126FFF   Kernel code/data/BSS (~1.2MB incl. static buffers)
0x000A0000 - 0x000FFFFF   BIOS/VGA/ROM (VGA text at 0xB8000)
0x001F0000                Kernel stack (grows downward)
0x00200000 - 0x002FFFFF   Userspace program area (1MB)
0x00300000 - 0x00300FFF   Userspace stack (4KB)
0x00300000 - 0x00FFFFFF   Free pages managed by PMM (~13MB)
```

## Boot Process

1. BIOS loads bootloader (512 bytes) at 0x7C00
2. Bootloader loads kernel from disk to 0x1000 (53 sectors)
3. Bootloader sets up GDT and switches to 32-bit protected mode
4. Kernel entry sets up stack at 0x1F0000, calls `kernel_main()`
5. Kernel initializes GDT with user-mode segments and TSS
6. Kernel initializes drivers (VGA, serial, keyboard, IDE, FAT32)
7. Kernel initializes IDT, remaps PIC, starts PIT timer, enables interrupts
8. Kernel initializes PMM, heap, and paging (identity-mapped 0-16MB)
9. Kernel launches userspace shell in ring 3 via `iret`

## Syscalls

Programs invoke syscalls via `int $0x80` with arguments in registers (EAX=number, EBX/ECX/EDX=args):

| # | Name | Description |
|---|------|-------------|
| 1 | print | Print string to console |
| 2 | exit | Exit program |
| 3 | file_open | Open file by name |
| 4 | file_read | Read from open file |
| 5 | file_close | Close file |
| 6 | list_dir | List directory entries |
| 7 | get_args | Get command-line arguments |
| 8 | getchar | Read character (blocking) |
| 9 | exec | Execute program |
| 10 | clear | Clear screen |
| 11 | sleep | Sleep N milliseconds |
| 12 | uptime | Get uptime in milliseconds |
| 13 | meminfo | Get PMM statistics |
| 14 | heap_stats | Get heap statistics |

## Adding Files to the Disk

```bash
mcopy -i hdd.img myfile.txt ::MYFILE.TXT
```

FAT32 uses uppercase 8.3 filenames.

## License

Educational code. Use freely.
