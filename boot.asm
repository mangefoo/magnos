; Simple bootloader for x86 OS
; This loads the kernel from disk and jumps to it

[BITS 16]
[ORG 0x7c00]

KERNEL_OFFSET equ 0x1000  ; Load kernel at 1MB mark

start:
    ; Save boot drive (BIOS passes it in DL)
    mov [BOOT_DRIVE], dl

    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00  ; Stack grows downward from bootloader

    ; Print loading message
    mov si, msg_loading
    call print_string

    ; Load kernel from disk
    call load_kernel

    ; Print success message
    mov si, msg_loaded
    call print_string

    ; Switch to protected mode
    call switch_to_pm

    ; Should never reach here
    jmp $

; Print string function (SI = pointer to null-terminated string)
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Load kernel from disk
load_kernel:
    pusha
    mov bx, KERNEL_OFFSET   ; Destination
    mov dh, 30              ; Load 30 sectors (15KB, enough for kernel)
    mov dl, [BOOT_DRIVE]    ; Boot drive from BIOS
    call disk_load
    popa
    ret

; Disk load function
; DH = number of sectors, DL = drive, BX = destination
disk_load:
    pusha
    push dx

    mov ah, 0x02            ; Read sectors function
    mov al, dh              ; Number of sectors to read
    mov ch, 0x00            ; Cylinder 0
    mov dh, 0x00            ; Head 0
    mov cl, 0x02            ; Start from sector 2 (after boot sector)

    int 0x13                ; BIOS disk interrupt
    jc disk_error           ; Jump if error (carry flag set)

    pop dx
    cmp al, dh              ; Check if all sectors read
    jne disk_error
    popa
    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $

; Switch to protected mode
switch_to_pm:
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load GDT

    ; Enable protected mode
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Far jump to clear pipeline and enter 32-bit mode
    jmp CODE_SEG:init_pm

[BITS 32]
init_pm:
    ; Set up segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp

    ; Jump to kernel
    call KERNEL_OFFSET

    ; Hang if kernel returns
    jmp $

; GDT
gdt_start:
    ; Null descriptor
    dq 0x0

gdt_code:
    ; Code segment descriptor
    dw 0xffff       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_data:
    ; Data segment descriptor
    dw 0xffff       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10010010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                 ; Address

; Constants
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; Messages
msg_loading db 'Loading kernel...', 0x0d, 0x0a, 0
msg_loaded db 'MagnOS loaded', 0x0d, 0x0a, 0
msg_disk_error db 'Disk read error!', 0x0d, 0x0a, 0

; Variables
BOOT_DRIVE db 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xaa55
