/* Linker script for the kernel */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1MB in physical memory */
    . = 0x1000;

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
}
