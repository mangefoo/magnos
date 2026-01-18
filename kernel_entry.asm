; Kernel entry point
; This is the first code that runs in the kernel

[BITS 32]
[EXTERN kernel_main]  ; Declare C function

global _start
_start:
    ; Segments should already be set up by bootloader
    ; But let's make sure stack is correct
    mov ebp, 0x90000
    mov esp, ebp

    ; Disable interrupts (we have no IDT)
    cli

    ; Write test message to VGA
    mov edi, 0xB8000
    mov eax, 0x0F540F4B  ; 'KT' in white
    mov [edi], eax

    call kernel_main  ; Call C kernel

    ; Hang forever
    cli
    hlt
.hang:
    jmp .hang
