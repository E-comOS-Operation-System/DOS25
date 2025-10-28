[BITS 16]
[ORG 0x20000]

; DOS25 Rescue System Boot

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x9000
sti

; Display rescue banner
mov si, rescue_banner
call print_str

; Check rescue mode flag
cmp word [0x500], 0xAA55
jne .invalid_entry

; Load rescue kernel
mov si, msg_loading
call print_str

mov ax, 0x3000
mov es, ax
xor bx, bx
mov ah, 0x02
mov al, 20      ; 20 sectors for rescue kernel
mov ch, 0
mov cl, 80      ; Sector 80 starts rescue kernel
mov dh, 0
mov dl, 0x80
int 0x13
jc .disk_error

; Jump to rescue kernel
jmp 0x3000:0x0000

.invalid_entry:
    mov si, msg_invalid
    call print_str
    hlt

.disk_error:
    mov si, msg_error
    call print_str
    hlt

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

rescue_banner   db 'DOS25 Rescue System', 0x0D, 0x0A
                db '===================', 0x0D, 0x0A, 0x0A, 0
msg_loading     db 'Loading rescue kernel...', 0x0D, 0x0A, 0
msg_invalid     db 'Invalid rescue entry!', 0
msg_error       db 'Rescue system load failed!', 0

times 512 - ($ - $$) db 0
