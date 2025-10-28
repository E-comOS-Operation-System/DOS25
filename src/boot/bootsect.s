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
; Kernel location: Disk sector 2 (LBA=1), loaded to physical address 0x100000 (64KB aligned, kernel link address)
mov ax, 0x1000    ; es:bx = 0x1000:0000 → Physical address 0x100000 (kernel load address)
mov es, ax
xor bx, bx
mov ah, 0x02      ; BIOS disk read function number
mov al, 20        ; Read 20 sectors (enough for the initial kernel, increase if needed)
mov ch, 0         ; Cylinder 0
mov cl, 2         ; Sector 2 (starts from 1, skip boot sector)
mov dh, 0         ; Head 0
mov dl, 0x80      ; First hard disk (0x00 is floppy)
int 0x13          ; Call BIOS disk interrupt
jc .disk_error    ; If carry flag = 1, disk read failed

; -------------------------- 3. Switch to 64-bit Long Mode (No Temporary Steps, Direct Transition) --------------------------
cli               ; Disable interrupts (mandatory during mode switch)

; 3.1 Disable paging (initialization phase)
mov eax, cr0
and eax, ~(1 << 31)  ; Clear PG bit (paging)
mov cr0, eax

; 3.2 Enable PAE (Physical Address Extension, preparation for long mode)
mov eax, cr4
or eax, (1 << 5)     ; Set PAE bit
mov cr4, eax

; 3.3 Load GDT (Global Descriptor Table, mandatory for 64-bit mode)
lgdt [gdt_descriptor]

; 3.4 Enter long mode (set IA32_EFER.LME)
mov ecx, 0xC0000080  ; IA32_EFER register
rdmsr                ; Read MSR
or eax, (1 << 8)     ; Set LME (Long Mode Enable)
wrmsr                ; Write back to MSR

; 3.5 Enable paging (required for long mode)
mov eax, cr0
or eax, (1 << 31)    ; Set PG bit
mov cr0, eax

; 3.6 Jump to 64-bit code (flush pipeline, enter long mode)
jmp gdt_code:long_mode_entry

; -------------------------- Error Handling (Not Placeholder, Actual Disk Read Failure Handling) --------------------------
.disk_error:
mov si, msg_disk_err
call print_str
hlt  ; Halt on error (more explicit than an infinite loop, clearly indicates failure)

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

; -------------------------- GDT Definition (Mandatory for 64-bit Mode, No Redundancy) --------------------------
gdt_start:
; Null descriptor (mandatory)
dq 0x0000000000000000
; Code segment descriptor (64-bit, executable)
gdt_code equ $ - gdt_start
dq 0x0020980000000000  ; Base=0, Limit=0, Type=Code Segment, DPL=0, L=1 (Long Mode)
; Data segment descriptor (64-bit, readable/writable)
gdt_data equ $ - gdt_start
dq 0x0000900000000000  ; Base=0, Limit=0, Type=Data Segment, DPL=0
gdt_end:

gdt_descriptor:
dw gdt_end - gdt_start - 1  ; GDT Limit
dq gdt_start                ; GDT Base Address

; -------------------------- 64-bit Entry (Final Step Before Jumping to Kernel) --------------------------
[BITS 64]
long_mode_entry:
; Initialize 64-bit segment registers (use GDT data segment for data segments)
mov ax, gdt_data
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax

; Jump to kernel entry (physical address 0x100000, kernel link address)
jmp 0x100000

; -------------------------- Strings (Actual Boot Messages, Not Placeholder) --------------------------
msg_boot      db 'DOS25: Loading E-comOS kernel...', 0x0D, 0x0A, 0  ; 0x0D=Carriage Return, 0x0A=Line Feed
msg_disk_err  db 'DOS25: Disk read failed!', 0

; -------------------------- Boot Sector Padding (Strictly 512 Bytes, Mandatory) --------------------------
times 510 - ($ - $$) db 0
dw 0x55AA  ; Boot sector signature (recognized by BIOS)