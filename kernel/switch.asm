[bits 32]

; void context_switch(uint32_t *old_esp, uint32_t new_esp)
; Saves current context, switches stack, restores new context
global context_switch
context_switch:
    pusha                   ; Save 8 GPRs (32 bytes)
                            ; Total: 32 bytes + return addr (4) = 36

    mov eax, [esp + 36]     ; arg1: pointer to old process's esp field
    mov ecx, [esp + 40]     ; arg2: new process's esp value
    mov [eax], esp          ; Save current ESP to old process
    mov esp, ecx            ; Switch to new process's stack

    popa                    ; Restore GPRs
    sti                     ; Always enable interrupts (sti takes effect after next insn)
    ret                     ; Jump to new process
