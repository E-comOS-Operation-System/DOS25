/**
 * DOS25 - Disk On System 2025 Bootloader
 * Copyright (C) 2025,2026  E-comOS Operation System Project
 * Copyright (C) 2025,2026  Saladin5101
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file main.c
 * @brief DOS25 UEFI Bootloader - Main entry point
 * Date: Jan 14, 2026 By Saladin5101
 * This is the first function called by the UEFI firmware.
 * It orchestrates the entire boot process: hardware initialization,
 * kernel loading, and finally jumping to the kernel.
 */

/* Prevent header conflicts */

#define _STDINT_H
#define _CORE_CRT_H
#define _INTTYPES_H


/* GNU-EFI headers first */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#endif
/* Project headers */
#include "../../include/common.h"
#include "include/boot.h"

/* Function forward declarations */
boot_status_t boot_params_init(boot_params_t* params);
boot_status_t get_memory_map(boot_params_t* params);
boot_status_t graphics_init(boot_params_t* params);
boot_status_t load_kernel(boot_params_t* params);
boot_status_t show_boot_menu(boot_params_t* params);
boot_status_t prepare_kernel_handoff(boot_params_t* params);
boot_status_t jump_to_kernel(boot_params_t* params);
boot_status_t run_ipc_tests(boot_params_t* params);
void wait_for_keypress(void);

/**
 * @brief UEFI application entry point
 * 
 * @param ImageHandle Handle that identifies the loaded image
 * @param SystemTable Pointer to the EFI system table
 * @return EFI_STATUS Status code indicating success or failure
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    boot_status_t boot_status;
    boot_params_t boot_params;
    
    /* Initialize GNU-EFI library */
    InitializeLib(ImageHandle, SystemTable);
    
    /* Simple startup message using GNU-EFI Print */
    Print(L"\r\n\r\n=== DOS25 UEFI Bootloader Starting ===\r\n");
    Print(L"Version: %u\r\n", DOS25_RUNTIME_VERSION);
    Print(L"Build: %s %s\r\n", L"Jan 20 2026", L"20:40:00");
    Print(L"========================================\r\n\r\n");
    
    /* Step 1: Initialize boot parameters structure */
    boot_status = boot_params_init(&boot_params);
    if (boot_status != BOOT_SUCCESS) {
        Print(L"Error: Failed to initialize boot parameters\r\n");
        goto error_exit;
    }
    
    /* Step 2: Run IPC tests */
    Print(L"Running DOS25 Runtime Architecture tests...\r\n");
    boot_status = run_ipc_tests(&boot_params);
    if (boot_status != BOOT_SUCCESS) {
        Print(L"Warning: IPC tests failed\r\n");
    }
    
    /* Step 3: Transfer control to kernel (demo) */
    boot_status = jump_to_kernel(&boot_params);
    if (boot_status != BOOT_SUCCESS) {
        Print(L"Error: Kernel handoff failed\r\n");
        goto error_exit;
    }
    
    return EFI_SUCCESS;
    
error_exit:
    /* Wait for key press before returning to firmware */
    wait_for_keypress();
    return EFI_ABORTED;
}

/**
 * @brief Wait for key press using GNU-EFI standard functions
 */
void wait_for_keypress(void) {
    EFI_INPUT_KEY key;
    UINTN index;
    EFI_EVENT events[1];
    
    Print(L"Press any key to continue...\r\n");
    
    /* Wait for key press event */
    events[0] = ST->ConIn->WaitForKey;
    BS->WaitForEvent(1, events, &index);
    
    /* Read the key stroke */
    ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
}