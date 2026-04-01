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
; Floppy: 18 sectors/track, 2 heads. Boot sector is track 0, head 0, sector 1.
; Kernel starts at sector 2. First read: sectors 2-18 (17 sectors) from head 0.
; Second read: sectors 1-18 (18 sectors) from head 1.
; Third read: sectors 1-18 (18 sectors) from cylinder 1, head 0.
; Total: 17 + 18 + 18 = 53 sectors = 27136 bytes (enough for kernel)
load_kernel:
    pusha
    mov dl, [BOOT_DRIVE]

    ; Read 1: 17 sectors from CHS 0/0/2
    mov bx, KERNEL_OFFSET
    mov ah, 0x02
    mov al, 17             ; 17 sectors (2 through 18)
    mov ch, 0              ; cylinder 0
    mov cl, 2              ; start at sector 2
    mov dh, 0              ; head 0
    int 0x13
    jc disk_error

    ; Read 2: 18 sectors from CHS 0/1/1
    add bx, 17 * 512
    mov ah, 0x02
    mov al, 18
    mov ch, 0              ; cylinder 0
    mov cl, 1              ; sector 1
    mov dh, 1              ; head 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Read 3: 18 sectors from CHS 1/0/1
    add bx, 18 * 512
    mov ah, 0x02
    mov al, 18
    mov ch, 1              ; cylinder 1
    mov cl, 1              ; sector 1
    mov dh, 0              ; head 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

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
