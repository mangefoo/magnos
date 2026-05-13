; MagnOS bootloader — CHS reads kernel from floppy, sets a VBE LFB graphics
; mode, then enters 32-bit protected mode and jumps to the kernel at 0x10000.

[BITS 16]
[ORG 0x7c00]

KERNEL_SEG       equ 0x1000           ; load kernel at linear 0x10000
MODE_INFO_BLOCK  equ 0x8000           ; 256-byte VBE mode info buffer
BOOT_INFO        equ 0x9000           ; struct passed to kernel:
                                      ;   +0  dword pitch
                                      ;   +4  dword width
                                      ;   +8  dword height
                                      ;   +12 dword bpp
                                      ;   +16 dword lfb_phys
                                      ;   +20 dword mode_ok

start:
    mov [BOOT_DRIVE], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    call serial_setup
    mov al, 'B'
    call serial_putc

    call load_kernel

    mov al, 'L'
    call serial_putc

    call set_video_mode

    mov al, 'V'
    call serial_putc

    call switch_to_pm
    jmp $

;------------------------------------------------------------
; Minimal real-mode COM1 serial for debug breadcrumbs (38400 8N1)
;------------------------------------------------------------
serial_setup:
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8
    mov al, 3
    out dx, al
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x03
    out dx, al
    mov dx, 0x3FC
    mov al, 0x03
    out dx, al
    ret

; AL = char to send (preserved)
serial_putc:
    push dx
    push ax
    mov ah, al
.wait:
    mov dx, 0x3FD
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, 0x3F8
    mov al, ah
    out dx, al
    pop ax
    pop dx
    ret

;------------------------------------------------------------
; CHS load: 100 sectors starting after the boot sector, into 0x10000.
; Floppy is 18 sectors/track, 2 heads.
;------------------------------------------------------------
load_kernel:
    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx

    mov cx, 0x0002      ; cyl=0, sector=2
    mov dh, 0
    mov al, 17
    call read_track

    mov cx, 0x0001
    mov dh, 1
    mov al, 18
    call read_track

    mov cx, 0x0101
    mov dh, 0
    mov al, 18
    call read_track

    mov cx, 0x0101
    mov dh, 1
    mov al, 18
    call read_track

    mov cx, 0x0201
    mov dh, 0
    mov al, 18
    call read_track

    mov cx, 0x0201
    mov dh, 1
    mov al, 11
    call read_track
    ret

; Reads AL sectors at CHS (CH=cyl,CL=sec,DH=head) into ES:BX.
; Advances ES so the next call appends.
read_track:
    mov [sec_count], al
    mov dl, [BOOT_DRIVE]
    mov ah, 0x02
    mov al, [sec_count]
    int 0x13
    jc disk_error
    push ax
    movzx ax, byte [sec_count]
    shl ax, 5
    mov bx, es
    add bx, ax
    mov es, bx
    pop ax
    xor bx, bx
    ret

sec_count: db 0

disk_error:
    mov al, 'E'
    call serial_putc
    jmp $

;------------------------------------------------------------
; Set a VBE LFB mode and save info at BOOT_INFO.
; Tries 1024x768, 800x600, then 640x480 — all 32bpp candidates.
;------------------------------------------------------------
set_video_mode:
    xor ax, ax
    mov es, ax                       ; VBE buffers live at ES:DI, need ES=0
    mov dword [BOOT_INFO + 20], 0
    mov cx, 0x118
    call try_mode
    jnc .ok
    mov cx, 0x115
    call try_mode
    jnc .ok
    mov cx, 0x112
    call try_mode
.ok:
    ret

; CX = base mode number. Sets mode + LFB; on success records info, CF=0.
try_mode:
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10
    cmp ax, 0x004F
    jne .fail
    mov bx, cx
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .fail
    movzx eax, word [MODE_INFO_BLOCK + 0x10]
    mov [BOOT_INFO + 0], eax
    movzx eax, word [MODE_INFO_BLOCK + 0x12]
    mov [BOOT_INFO + 4], eax
    movzx eax, word [MODE_INFO_BLOCK + 0x14]
    mov [BOOT_INFO + 8], eax
    movzx eax, byte [MODE_INFO_BLOCK + 0x19]
    mov [BOOT_INFO + 12], eax
    mov eax, [MODE_INFO_BLOCK + 0x28]
    mov [BOOT_INFO + 16], eax
    mov dword [BOOT_INFO + 20], 1
    clc
    ret
.fail:
    stc
    ret

;------------------------------------------------------------
; Enter 32-bit protected mode and call into the kernel.
;------------------------------------------------------------
switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    call 0x10000
    jmp $

gdt_start:
    dq 0x0
gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

BOOT_DRIVE db 0

times 510-($-$$) db 0
dw 0xaa55
