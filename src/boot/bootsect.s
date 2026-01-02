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

; ================================================================
; DOS25 - Master Boot Record (MBR)
; Fixed version - Simplified and more reliable
; ================================================================
[BITS 16]               ; 16-bit real mode
[ORG 0x7C00]           ; BIOS loads MBR at 0x7C00

_start:
    ; Setup stack and segment registers
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    ; Save boot drive number
    mov [boot_drive], dl
    
    ; Clear screen and set 80x25 text mode
    mov ax, 0x0003
    int 0x10
    
    ; Display initial message
    mov si, msg_booting
    call print_string
    
    ; Load Stage 2 (sectors 2-14) to 0x8000
    mov si, msg_loading
    call print_string
    
    ; ES:BX = Destination address (0x0800:0x0000 = 0x8000)
    mov ax, 0x0800
    mov es, ax
    xor bx, bx          ; BX = 0x0000
    
    ; Set up disk read parameters
    mov ah, 0x02        ; BIOS function: read sectors
    mov al, 13          ; Number of sectors to read
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Starting from sector 2
    mov dh, 0           ; Head 0
    mov dl, [boot_drive] ; Drive number
    int 0x13            ; Call BIOS disk service
    
    ; Check for read errors
    jc disk_error
    
    ; Verify number of sectors read
    cmp al, 13
    jne sector_error
    
    ; Debug: Display what we loaded
    mov si, msg_verify
    call print_string
    
    ; Check if we have valid code at 0x8000
    ; Look for non-zero bytes in the first 8 bytes
    mov cx, 8
    xor bx, bx
    mov di, 0           ; Counter for non-zero bytes
check_loop:
    mov al, [es:bx]
    cmp al, 0
    je .skip
    inc di              ; Count non-zero bytes
.skip:
    inc bx
    loop check_loop
    
    ; If all bytes are zero, it's an error
    cmp di, 0
    je zero_error
    
    ; Success - jump to stage 2
    mov si, msg_success
    call print_string
    call newline
    
    ; CRITICAL FIX: Reset segment registers before jump
    xor ax, ax
    mov ds, ax
    mov es, ax
    
    ; Jump to Stage 2 at 0x0800:0x0000
    jmp 0x0800:0x0000

; ============================================================
; Error handlers
; ============================================================
disk_error:
    mov si, msg_disk_error
    call print_string
    jmp hang

sector_error:
    mov si, msg_sector_error
    call print_string
    jmp hang

zero_error:
    mov si, msg_zero_error
    call print_string
    jmp hang

hang:
    mov si, msg_halted
    call print_string
    cli
.hang_loop:
    hlt
    jmp .hang_loop

; ============================================================
; Utility Functions
; ============================================================

; Print string
; Input: SI = pointer to string
print_string:
    pusha
    mov ah, 0x0E
    mov bx, 0x0007
.loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Print newline
newline:
    push ax
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    pop ax
    ret

; ============================================================
; Data Section
; ============================================================
boot_drive: db 0

msg_booting:     db "S1: Booting...", 0x0D, 0x0A, 0
msg_loading:     db "Loading stage 2...", 0x0D, 0x0A, 0
msg_verify:      db "Verifying...", 0x0D, 0x0A, 0
msg_success:     db "Jumping to 0x8000", 0x0D, 0x0A, 0
msg_disk_error:  db "Disk error!", 0x0D, 0x0A, 0
msg_sector_error:db "Sector error!", 0x0D, 0x0A, 0
msg_zero_error:  db "No data at 0x8000!", 0x0D, 0x0A, 0
msg_halted:      db "Halted.", 0x0D, 0x0A, 0

; ============================================================
; Boot signature
; ============================================================
times 510-($-$$) db 0
dw 0xAA55