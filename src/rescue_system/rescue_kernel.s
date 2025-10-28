[BITS 16]
[ORG 0x30000]

; DOS25 Rescue Kernel

start:
    mov si, banner
    call print
    
    mov si, menu
    call print
    
    ; Wait for command
    mov si, prompt
    call print
    
    ; Auto-install from sector 200
    mov si, msg_install
    call print
    
    call install_system
    
    mov si, msg_done
    call print
    
.halt:
    hlt
    jmp .halt

; Print string
print:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; Install system from DSF at sector 200
install_system:
    ; Read DSF header
    mov ax, 0x5000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 200
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc .error
    
    ; Check signature "DSF"
    mov si, 0x50000
    cmp byte [si], 'D'
    jne .error
    cmp byte [si+1], 'S'
    jne .error
    cmp byte [si+2], 'F'
    jne .error
    
    ; Write boot sector to sector 2
    mov ax, 0x5000
    mov es, ax
    mov bx, 512
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 201
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc .error
    
    ; Write to sector 2
    mov ah, 0x03
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc .error
    
    ret

.error:
    mov si, msg_error
    call print
    ret

banner      db 'DOS25 Rescue System', 0x0D, 0x0A
            db '===================', 0x0D, 0x0A, 0x0A, 0
menu        db 'Commands:', 0x0D, 0x0A
            db '  install - Install from DSF', 0x0D, 0x0A
            db '  reboot  - Reboot', 0x0D, 0x0A, 0x0A, 0
prompt      db 'rescue> ', 0
msg_install db 'install', 0x0D, 0x0A, 'Installing system...', 0x0D, 0x0A, 0
msg_done    db 'Installation complete!', 0x0D, 0x0A, 0
msg_error   db 'ERROR: Installation failed!', 0x0D, 0x0A, 0

times 512 - ($ - $$) db 0
