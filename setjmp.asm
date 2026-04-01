; setjmp/longjmp for MagnOS
; Used to save and restore execution context for nested exec
[bits 32]

section .text

; int exec_setjmp(exec_jmp_buf *buf)
; Saves callee-saved registers, stack pointer, and return address.
; Returns 0 on initial call, non-zero when restored via exec_longjmp.
global exec_setjmp
exec_setjmp:
    mov eax, [esp+4]        ; buf pointer (first argument)
    mov [eax+0],  ebx       ; save ebx
    mov [eax+4],  esi       ; save esi
    mov [eax+8],  edi       ; save edi
    mov [eax+12], ebp       ; save ebp
    lea ecx, [esp+4]        ; save esp as it will be after our ret
    mov [eax+16], ecx
    mov ecx, [esp]          ; save return address
    mov [eax+20], ecx
    xor eax, eax            ; return 0
    ret

; __attribute__((noreturn))
; void exec_longjmp(exec_jmp_buf *buf, int val)
; Restores saved context. exec_setjmp returns val (or 1 if val==0).
global exec_longjmp
exec_longjmp:
    mov edx, [esp+4]        ; buf pointer
    mov eax, [esp+8]        ; return value
    test eax, eax
    jnz .nonzero
    inc eax                 ; ensure non-zero return
.nonzero:
    mov ebx, [edx+0]        ; restore ebx
    mov esi, [edx+4]        ; restore esi
    mov edi, [edx+8]        ; restore edi
    mov ebp, [edx+12]       ; restore ebp
    mov esp, [edx+16]       ; restore esp
    jmp [edx+20]            ; jump to saved return address
