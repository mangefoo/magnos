/* Linker script for the kernel — loaded at linear 0x10000 by the bootloader */

ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    /* Page-aligned symbol marking end of kernel image */
    . = ALIGN(4096);
    _kernel_end = .;
}
