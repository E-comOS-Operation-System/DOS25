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
; DOS25 - Stage 2 Bootloader
; Loaded at physical address 0x8000 by Stage 1
; ================================================================
[BITS 16]               ; 16-bit real mode
[ORG 0x0000]           ; Critical fix: Use ORG 0x0000 instead of 0x8000
                       ; Because Stage 1 jumps to 0x0800:0x0000
                       ; CS=0x0800, IP=0x0000, so ORG should be 0x0000

; ================================================================
; Entry Point
; ================================================================
stage2_start:
    ; Immediately set up segment registers
    cli                 ; Disable interrupts
    mov ax, cs          ; Copy CS to DS, ES, SS
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack grows downward from 0x7C00
    sti                 ; Enable interrupts
    
    ; Now we can safely use BIOS interrupts
    ; Clear screen and set to 80x25 text mode
    mov ax, 0x0003      ; AH=0x00 (Set video mode), AL=0x03 (80x25)
    int 0x10
    
    ; Set text color to RED (for visual proof stage 2 is running)
    mov ah, 0x0B        ; BIOS function: Set color
    mov bh, 0x00        ; Page 0
    mov bl, 0x04        ; Color: Red
    int 0x10
    
    ; Print success message
    mov si, msg_success
    call print_string
    
    ; Display CS:IP to verify our location
    mov si, msg_cs_ip
    call print_string
    mov ax, cs
    call print_hex_word
    mov al, ':'         ; Separator
    call print_char
    mov ax, 0x0000      ; IP is 0x0000
    call print_hex_word
    call newline
    
    ; Halt the CPU
    cli
.hang:
    hlt
    jmp .hang

; ================================================================
; Functions
; ================================================================

; ----------------------------------------------------------------
; Print null-terminated string
; Input: SI = pointer to string
; ----------------------------------------------------------------
print_string:
    pusha
    mov ah, 0x0E        ; BIOS teletype function
    mov bh, 0x00        ; Page 0
    mov bl, 0x07        ; Light gray text
.print_loop:
    lodsb               ; Load byte from [SI] to AL, increment SI
    or al, al           ; Check for null terminator
    jz .print_done
    int 0x10            ; Print character
    jmp .print_loop
.print_done:
    popa
    ret

; ----------------------------------------------------------------
; Print single character
; Input: AL = character to print
; ----------------------------------------------------------------
print_char:
    push ax
    mov ah, 0x0E
    int 0x10
    pop ax
    ret

; ----------------------------------------------------------------
; Print newline (CR+LF)
; ----------------------------------------------------------------
newline:
    push ax
    mov ah, 0x0E
    mov al, 0x0D        ; Carriage return
    int 0x10
    mov al, 0x0A        ; Line feed
    int 0x10
    pop ax
    ret

; ----------------------------------------------------------------
; Print word in hexadecimal
; Input: AX = word to print
; ----------------------------------------------------------------
print_hex_word:
    pusha
    mov cx, 4           ; 4 hex digits
.hex_loop:
    rol ax, 4           ; Rotate left to get next nibble
    push ax
    and al, 0x0F         ; Mask lower nibble
    cmp al, 10
    jl .is_digit
    add al, 7           ; Adjust for A-F
.is_digit:
    add al, '0'         ; Convert to ASCII
    call print_char
    pop ax
    loop .hex_loop
    popa
    ret

; ================================================================
; Data Section
; ================================================================
msg_success:    db "Stage 2: SUCCESS! Running at 0x8000", 0x0D, 0x0A, 0
msg_cs_ip:      db "CS:IP = 0x", 0
msg_a20:        db "A20: Enabled", 0x0D, 0x0A, 0
msg_gdt:        db "GDT: Loaded", 0x0D, 0x0A, 0
msg_pm:         db "PM: Switching to protected mode...", 0x0D, 0x0A, 0

; ================================================================
; Padding
; Fill to 13 sectors (6656 bytes) as required
; ================================================================
times 6656-($-$$) db 0x90