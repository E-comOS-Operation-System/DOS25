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
[ORG 0x7C00]  ; BIOS loads the MBR to 0x7C00
; -------------------------- 1. Real Mode Initialization (Mandatory Preparations) --------------------------
xor ax, ax
mov ds, ax        ; Data segment = 0
mov es, ax        ; Extra segment = 0
mov ss, ax        ; Stack segment = 0
mov sp, 0x7C00    ; Stack top set above the bootloader (safe address)
sti               ; Enable interrupts (needed for subsequent disk reads)

; Display boot message (not temporary, part of the boot process)
mov si, msg_boot
call print_str

; -------------------------- 2. Load Kernel into Memory (Core Functionality, No Placeholder) --------------------------
; Kernel location: Disk sector 3, loaded to physical address 0x100000 (1MB)
; First load to low memory, then copy to 1MB in protected mode
mov ax, 0x1000    ; Temporary load to 0x10000
mov es, ax
xor bx, bx
mov ah, 0x02      ; BIOS disk read function number
mov al, 20        ; Read 20 sectors
mov ch, 0         ; Cylinder 0
mov cl, 3         ; Sector 3 (kernel starts here)
mov dh, 0         ; Head 0
mov dl, 0x00      ; Floppy disk
int 0x13          ; Call BIOS disk interrupt
jc .disk_error    ; If carry flag = 1, disk read failed

; Debug: Check if disk read succeeded
mov si, msg_disk_success
call print_str
jmp .continue_execution

.disk_error:
mov si, msg_disk_err
call print_str
hlt  ; Halt on error (more explicit than an infinite loop, clearly indicates failure)

.continue_execution:
; Display kernel loaded message
mov si, msg_loaded
call print_str

; -------------------------- 3. Switch to 64-bit Long Mode --------------------------
cli               ; Disable interrupts

; 3.1 Enable A20 line
in al, 0x92
or al, 2
out 0x92, al

; 3.2 Load GDT
lgdt [gdt_descriptor]

; 3.3 Enter protected mode
mov eax, cr0
or eax, 1
mov cr0, eax

jmp 0x08:protected_mode_32

; -------------------------- Utility Functions (Actually Usable, Not Temporary) --------------------------
; Print string (using BIOS int 10h, reliable in real mode)
print_str:
mov ah, 0x0E  ; BIOS Teletype mode
.repeat:
lodsb         ; Load character from [si] to al
test al, al   ; Check if 0 (end of string)
jz .done
int 0x10      ; Call BIOS to display
jmp .repeat
.done:
ret

; -------------------------- GDT Definition --------------------------
gdt_start:
; Null descriptor
dq 0x0000000000000000
; Code segment (64-bit)
gdt_code equ $ - gdt_start
dq 0x0020980000000000
; Data segment
gdt_data equ $ - gdt_start
dq 0x0000920000000000
gdt_end:

gdt_descriptor:
dw gdt_end - gdt_start - 1
dd gdt_start



; -------------------------- Strings (Actual Boot Messages, Not Placeholder) --------------------------
[BITS 32]
protected_mode_32:
; Setup segments
mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000

; Show "32" to indicate 32-bit mode works
mov edi, 0xB8000
mov eax, 0x07320733
mov [edi], eax

; Skip copy, run kernel at 0x10000
; Show "SK" to indicate skip copy
mov edi, 0xB8004
mov eax, 0x074B0753
mov [edi], eax

; Setup page tables for long mode
mov edi, 0x1000
mov cr3, edi
xor eax, eax
mov ecx, 4096
rep stosd
mov edi, cr3

; PML4[0] -> PDPT
mov dword [edi], 0x2003
; PDPT[0] -> PDT
mov dword [edi + 0x1000], 0x3003
; PDT[0] -> 2MB page
mov dword [edi + 0x2000], 0x83

; Enable PAE
mov eax, cr4
or eax, 1 << 5
mov cr4, eax

; Enable long mode
mov ecx, 0xC0000080
rdmsr
or eax, 1 << 8
wrmsr

; Enable paging
mov eax, cr0
or eax, 1 << 31
mov cr0, eax

; Jump to 64-bit code
jmp gdt_code:long_mode

[BITS 64]
long_mode:
mov ax, gdt_data
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax

; Jump to kernel at 0x10000
jmp 0x10000

[BITS 16]
msg_boot      db 'DOS25: Loading E-comOS kernel...', 0x0D, 0x0A, 0
msg_loaded    db 'DOS25: Kernel loaded, entering 64-bit mode...', 0x0D, 0x0A, 0
msg_disk_err  db 'DOS25: Disk read failed!', 0
msg_disk_success db 'DOS25: Disk read succeeded!', 0

; -------------------------- Boot Sector Padding (Strictly 512 Bytes, Mandatory) --------------------------
times 510 - ($ - $$) db 0
dw 0xAA55  ; Boot sector signature (recognized by BIOS)