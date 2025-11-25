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

start:
    ; Init screen
    call clear_screen
    call display_menu

menu_loop:
    ; Wait user's input
    call get_user_input
    cmp al, '1'
    je boot_normal
    cmp al, '2'
    je boot_bios_compat
    cmp al, '3'
    je boot_rescue
    jmp menu_loop

boot_normal:
    ; Start
    call load_kernel
    jmp start_kernel

boot_bios_compat:
    ; BIOS Mode
    call load_bios_compat
    jmp start_kernel

boot_rescue:
    ; "Help me" screen 
    call enter_rescue_mode
    jmp start

; Show menu
display_menu:
    mov si, menu_text
    call print_string
    ret

menu_text db "DOS25 Start Menu", 0x0D, 0x0A
          db "1. Start to system", 0x0D, 0x0A
          db "2. BIOS MODE", 0x0D, 0x0A
          db "3. HELP ME SCREEN", 0x0D, 0x0A
          db "Please choose one: ", 0


debug_message db "Load failed : CANNOT FOUND KERNEL!!!", 0x0D, 0x0A, 0
kernel_load_message db "Loading kernel...", 0x0D, 0x0A, 0
kernel_loaded_message db "Kernel loaded!", 0x0D, 0x0A, 0

load_kernel:
    mov si, kernel_load_message
    call print_string

    ; Kernel is in disk's sector 3
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 20       ; 20 sectors
    mov ch, 0
    mov cl, 3        ; Sector 3 (kernel location)
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc kernel_load_failed

    mov si, kernel_loaded_message
    call print_string
    ret

kernel_load_failed:
    mov si, debug_message
    call print_string
    hlt

; Placeholder functions
clear_screen:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret

get_user_input:
    mov ah, 0x00
    int 0x16
    ret

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

load_bios_compat:
    ; TODO: BIOS compatibility mode
    ret

enter_rescue_mode:
    ; TODO: Enter rescue mode
    ret

start_kernel:
    ; Jump to kernel at 0x10000
    jmp 0x1000:0x0000 