[BITS 16]
[ORG 0x7C00]

; DOS25 Boot Menu - Stage 1
; This loads the original bootsect.s or rescue system based on user choice

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00
sti

; Display boot menu
call clear_screen
mov si, menu_header
call print_str

mov si, menu_options
call print_str

; Wait for user input with timeout
mov cx, 50          ; 5 seconds timeout (50 * 0.1s)
.wait_key:
    mov ah, 0x01    ; Check keyboard buffer
    int 0x16
    jnz .key_pressed
    
    ; Delay 0.1 second
    mov ah, 0x86
    mov cx, 0x01
    mov dx, 0x86A0
    int 0x15
    
    loop .wait_key
    
    ; Timeout - default to normal boot
    jmp normal_boot

.key_pressed:
    mov ah, 0x00    ; Read key
    int 0x16
    
    cmp al, '1'
    je normal_boot
    cmp al, '2'
    je rescue_boot
    cmp al, 0x0D    ; Enter key
    je normal_boot
    
    jmp .wait_key

normal_boot:
    mov si, msg_normal
    call print_str
    
    ; Load original bootsect.s from sector 2
    mov ax, 0x07E0  ; Load to 0x7E00 (right after this bootloader)
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 1       ; 1 sector
    mov ch, 0
    mov cl, 2       ; Sector 2 contains original bootsect.s
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc disk_error
    
    ; Jump to original bootsect.s
    jmp 0x07E0:0x0000

rescue_boot:
    mov si, msg_rescue
    call print_str
    
    ; Set rescue mode flag
    mov word [0x500], 0xAA55
    
    ; Load rescue system from sector 50
    mov ax, 0x2000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 30      ; 30 sectors for rescue system
    mov ch, 0
    mov cl, 50      ; Sector 50 starts rescue system
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc disk_error
    
    ; Jump to rescue system
    jmp 0x2000:0x0000

disk_error:
    mov si, msg_disk_err
    call print_str
    hlt

clear_screen:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret

print_str:
    mov ah, 0x0E
.repeat:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .repeat
.done:
    ret

; Strings
menu_header     db 'DOS25 - Disk On System 2025', 0x0D, 0x0A
                db '============================', 0x0D, 0x0A, 0x0A, 0
menu_options    db '1. Normal Boot (E-comOS)', 0x0D, 0x0A
                db '2. Rescue Mode', 0x0D, 0x0A, 0x0A
                db 'Select (1-2, default in 5s): ', 0
msg_normal      db 0x0D, 0x0A, 'Booting E-comOS...', 0x0D, 0x0A, 0
msg_rescue      db 0x0D, 0x0A, 'Entering Rescue Mode...', 0x0D, 0x0A, 0
msg_disk_err    db 'DOS25: Disk read error!', 0

times 510 - ($ - $$) db 0
dw 0x55AA
