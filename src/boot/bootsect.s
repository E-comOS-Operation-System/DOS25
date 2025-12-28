; DOS25 - Disk On System 2025 
; Copyright (C) 2025 Saladin5101
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    mov [boot_drive], dl
    
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Print boot message
    mov si, msg_loading
    call print_string
    
    ; Load stage 2 (13 sectors) to 0x8000
    mov ax, 0x0800
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02        ; Read sectors
    mov al, 13          ; Number of sectors to read
    mov ch, 0           ; Cylinder
    mov cl, 2           ; Sector (starting from 2)
    mov dh, 0           ; Head
    mov dl, [boot_drive] ; Drive
    int 0x13
    jc disk_error
    
    ; Jump to stage 2
    mov si, msg_jump
    call print_string
    jmp 0x0800:0x0000

disk_error:
    mov si, msg_error
    call print_string
    mov al, ah          ; Error code
    call print_hex_byte
    jmp $

print_string:
    pusha
    mov ah, 0x0E
.print_loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .print_loop
.done:
    popa
    ret

print_hex_byte:
    pusha
    mov bl, al
    shr al, 4
    call .nibble
    mov al, bl
    and al, 0x0F
    call .nibble
    popa
    ret
.nibble:
    cmp al, 10
    jl .digit
    add al, 7
.digit:
    add al, '0'
    mov ah, 0x0E
    int 0x10
    ret

boot_drive: db 0
msg_loading: db "Booting DOS25...", 0x0D, 0x0A, 0
msg_jump: db "Loading stage2...", 0x0D, 0x0A, 0
msg_error: db "Disk error: 0x", 0

times 510-($-$$) db 0
dw 0xAA55