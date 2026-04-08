[bits 32]

; Load new GDT and reload segment registers
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]     ; Get GDT pointer argument
    lgdt [eax]             ; Load GDT

    ; Reload data segments with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS with kernel code selector (0x08)
    jmp 0x08:.flush
.flush:
    ret

; Load TSS register
global tss_flush
tss_flush:
    mov ax, 0x28           ; TSS selector (GDT entry 5)
    ltr ax
    ret
