; ISR stubs for MagnOS
; 48 entry points (exceptions 0-31, IRQs 32-47) + common handler stub

[bits 32]
[extern isr_handler]

; Macro for exceptions that do NOT push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0            ; dummy error code
    push dword %1           ; interrupt number
    jmp isr_common_stub
%endmacro

; Macro for exceptions that DO push an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1           ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; CPU exceptions 0-31
ISR_NOERRCODE 0     ; Division by Zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; NMI
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE   8     ; Double Fault
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment Not Present
ISR_ERRCODE   12    ; Stack-Segment Fault
ISR_ERRCODE   13    ; General Protection Fault
ISR_ERRCODE   14    ; Page Fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 FPU Error
ISR_ERRCODE   17    ; Alignment Check
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD FPU Exception
ISR_NOERRCODE 20    ; Virtualization Exception
ISR_ERRCODE   21    ; Control Protection
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Reserved
ISR_NOERRCODE 29    ; Reserved
ISR_NOERRCODE 30    ; Reserved
ISR_NOERRCODE 31    ; Reserved

; IRQs 0-15 (mapped to interrupts 32-47)
ISR_NOERRCODE 32    ; IRQ0 - PIT Timer
ISR_NOERRCODE 33    ; IRQ1 - Keyboard
ISR_NOERRCODE 34    ; IRQ2 - Cascade
ISR_NOERRCODE 35    ; IRQ3 - COM2
ISR_NOERRCODE 36    ; IRQ4 - COM1
ISR_NOERRCODE 37    ; IRQ5 - LPT2
ISR_NOERRCODE 38    ; IRQ6 - Floppy
ISR_NOERRCODE 39    ; IRQ7 - LPT1 / Spurious
ISR_NOERRCODE 40    ; IRQ8 - CMOS RTC
ISR_NOERRCODE 41    ; IRQ9 - Free
ISR_NOERRCODE 42    ; IRQ10 - Free
ISR_NOERRCODE 43    ; IRQ11 - Free
ISR_NOERRCODE 44    ; IRQ12 - PS/2 Mouse
ISR_NOERRCODE 45    ; IRQ13 - FPU
ISR_NOERRCODE 46    ; IRQ14 - Primary ATA
ISR_NOERRCODE 47    ; IRQ15 - Secondary ATA

; Syscall interrupt (int 0x80)
ISR_NOERRCODE 128   ; Syscall

; Common stub: saves state, calls C handler, restores and irets
isr_common_stub:
    pusha               ; Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    mov ax, ds
    push eax            ; Save data segment (as 32-bit for alignment)

    mov ax, 0x10        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pass pointer to isr_regs struct
    call isr_handler
    add esp, 4          ; Clean up argument

    pop eax             ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Restore registers
    add esp, 8          ; Clean up int_no and err_code
    iret
