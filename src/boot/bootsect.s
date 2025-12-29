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

; ========================================================
; Master Boot Record (MBR) - bootsect.s
; Loads stage 2 (13 sectors) to 0x8000
; ========================================================

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    ; Save boot drive
    mov [boot_drive], dl
    
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Print message
    mov si, msg_boot
    call print_string
    
    ; ============================================
    ; CRITICAL: Load 13 sectors, NOT 4!
    ; Stage 2 is 6656 bytes = 13 sectors
    ; ============================================
    mov ax, 0x0800      ; Segment 0x0800
    mov es, ax          ; ES = 0x0800
    xor bx, bx          ; Offset 0x0000
                        ; Physical address = 0x0800:0x0000 = 0x8000
    
    mov ah, 0x02        ; BIOS read sectors
    mov al, 13          ; NUMBER OF SECTORS TO LOAD: 13
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2 (sector 1 is MBR)
    mov dh, 0           ; Head 0
    mov dl, [boot_drive] ; Drive
    int 0x13
    
    ; Check for error
    jc disk_error
    
    ; Verify we loaded 13 sectors
    cmp al, 13
    jne disk_error
    
    ; Success - jump to stage 2
    mov si, msg_success
    call print_string
    
    ; CRITICAL: This jumps to 0x8000
    jmp 0x0800:0x0000

disk_error:
    mov si, msg_error
    call print_string
    
    ; Display error code
    mov al, ah
    call print_hex_byte
    
    ; Hang
    cli
.hang:
    hlt
    jmp .hang

; ============================================
; Functions
; ============================================
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

; ============================================
; Data
; ============================================
boot_drive:     db 0
msg_boot:       db "Stage 1: Booting DOS25...", 0x0D, 0x0A, 0
msg_success:    db "Stage 1: Loading 13 sectors to 0x8000...", 0x0D, 0x0A, 0
msg_error:      db "Stage 1: Disk error 0x", 0

; ============================================
; Boot signature
; ============================================
times 510-($-$$) db 0
dw 0xAA55