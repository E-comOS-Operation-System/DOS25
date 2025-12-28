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
; Stage 2 Bootloader
; ================================================================
; Loaded at 0x8000 by the MBR (bootsect.s)
; Responsible for:
; 1. Setting up a protected mode environment
; 2. Loading the kernel
; 3. Jumping to the kernel
; ================================================================

[BITS 16]               ; 16-bit real mode
[ORG 0x8000]           ; Stage 2 is loaded at physical address 0x8000

; ================================================================
; Entry Point
; ================================================================
stage2_start:
    cli                 ; Disable interrupts
    xor ax, ax          ; Clear AX register
    mov ds, ax          ; Set DS to 0
    mov es, ax          ; Set ES to 0
    mov ss, ax          ; Set SS to 0
    mov sp, 0x7C00      ; Set stack pointer to 0x7C00
    sti                 ; Enable interrupts
    
    ; Clear screen and set text mode
    mov ax, 0x0003      ; 80x25 text mode
    int 0x10
    
    ; Set text color to light green
    mov ax, 0x0A00
    int 0x10
    
    ; ============================================================
    ; Debug Message: Show that we're running at 0x8000
    ; ============================================================
    mov si, msg_check_addr
    call print_string
    
    ; ============================================================
    ; Enable A20 Line
    ; Required to access memory above 1MB in protected mode
    ; ============================================================
enable_a20:
    call wait_for_input
    mov al, 0xD1
    out 0x64, al
    call wait_for_input
    mov al, 0xDF
    out 0x60, al
    call wait_for_input
    
    mov si, msg_a20_enabled
    call print_string
    
    ; ============================================================
    ; Load GDT (Global Descriptor Table)
    ; Required for protected mode operation
    ; ============================================================
    lgdt [gdt_descriptor]   ; Load GDT descriptor
    mov si, msg_gdt_loaded
    call print_string
    
    ; ============================================================
    ; Switch to Protected Mode
    ; ============================================================
    mov si, msg_enter_pm
    call print_string
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Far jump to flush CPU pipeline and load CS with 32-bit code segment
    jmp CODE_SEG:init_protected_mode

; ================================================================
; Functions
; ================================================================

; ----------------------------------------------------------------
; Print String
; ----------------------------------------------------------------
; Input:  SI = pointer to null-terminated string
; Output: Prints string to screen
; ----------------------------------------------------------------
print_string:
    pusha
    mov ah, 0x0E        ; BIOS teletype function
.print_loop:
    lodsb               ; Load next character
    or al, al           ; Check for null terminator
    jz .print_done
    int 0x10            ; Print character
    jmp .print_loop
.print_done:
    popa
    ret

; ----------------------------------------------------------------
; Wait for Input Buffer
; ----------------------------------------------------------------
; Required for A20 line enabling
; ----------------------------------------------------------------
wait_for_input:
    in al, 0x64
    test al, 0x02
    jnz wait_for_input
    ret

; ================================================================
; Data Section
; ================================================================

; Messages
msg_check_addr:     db "Stage 2: Running at 0x8000", 0x0D, 0x0A, 0
msg_a20_enabled:    db "A20 Line: Enabled", 0x0D, 0x0A, 0
msg_gdt_loaded:     db "GDT: Loaded", 0x0D, 0x0A, 0
msg_enter_pm:       db "PM: Switching to protected mode...", 0x0D, 0x0A, 0
msg_kernel_loaded:  db "Kernel: Loading from sector 15...", 0x0D, 0x0A, 0
msg_jump_to_kernel: db "Jumping to kernel at 0x100000...", 0x0D, 0x0A, 0

; ================================================================
; Protected Mode Code (32-bit)
; ================================================================
[BITS 32]

; ----------------------------------------------------------------
; Protected Mode Initialization
; ----------------------------------------------------------------
init_protected_mode:
    ; Set up segment registers for protected mode
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov ebp, 0x90000    ; Stack base pointer
    mov esp, ebp        ; Stack pointer
    
    mov esi, msg_pm_active
    call print_string_pm
    
    ; ============================================================
    ; Load Kernel
    ; Kernel starts at sector 15 (after MBR and Stage 2)
    ; ============================================================
    mov esi, msg_load_kernel
    call print_string_pm
    
    ; We'll need to load the kernel from disk
    ; This requires writing a disk driver in protected mode
    ; For now, we'll just hang
    
    ; ============================================================
    ; Hang (temporary - will be replaced with kernel loader)
    ; ============================================================
    cli
.hang:
    hlt
    jmp .hang

; ----------------------------------------------------------------
; Print String in Protected Mode
; ----------------------------------------------------------------
; Input:  ESI = pointer to null-terminated string
; Output: Prints to video memory at 0xB8000
; ----------------------------------------------------------------
print_string_pm:
    pusha
    mov edx, 0xB8000    ; Video memory address
    mov ah, 0x0F        ; White text on black background
    
.print_loop_pm:
    lodsb               ; Load next character
    or al, al           ; Check for null terminator
    jz .print_done_pm
    
    mov [edx], ax       ; Store character and attribute
    add edx, 2          ; Move to next character position
    
    jmp .print_loop_pm
    
.print_done_pm:
    popa
    ret

; ================================================================
; GDT (Global Descriptor Table)
; ================================================================
gdt_start:
    ; Null descriptor (required)
    dq 0x0000000000000000
    
    ; Code segment descriptor
gdt_code:
    dw 0xFFFF           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 10011010b        ; Access byte
    ;   P=1, DPL=00, S=1, E=1, DC=0, RW=1, A=0
    db 11001111b        ; Flags + Limit (bits 16-19)
    ;   G=1, D/B=1, L=0, AVL=0, Limit=1111
    db 0x00             ; Base (bits 24-31)
    
    ; Data segment descriptor
gdt_data:
    dw 0xFFFF           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 10010010b        ; Access byte
    ;   P=1, DPL=00, S=1, E=0, DC=0, RW=1, A=0
    db 11001111b        ; Flags + Limit (bits 16-19)
    ;   G=1, D/B=1, L=0, AVL=0, Limit=1111
    db 0x00             ; Base (bits 24-31)
gdt_end:

; ================================================================
; GDT Descriptor
; ================================================================
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size of GDT
    dd gdt_start                ; Address of GDT

; ================================================================
; Segment Selectors
; ================================================================
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ================================================================
; Protected Mode Messages
; ================================================================
msg_pm_active:      db "Protected Mode: Active", 0
msg_load_kernel:    db "Kernel: Loading...", 0
msg_error:          db "Error: Kernel not found", 0

; ================================================================
; Padding
; Pad the file to 13 sectors (6656 bytes) as required
; Sector 1: MBR (bootsect.s)
; Sectors 2-14: Stage 2 (this file)
; Sectors 15+: Kernel
; ================================================================
times 6656-($-$$) db 0x90  ; Fill with NOP instructions