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
 * @file boot.c
 * @brief DOS25 UEFI Bootloader - Core boot logic
 * 
 * Implements the main boot sequence: hardware detection, kernel loading,
 * and transferring control to the 64-bit kernel.
 */

/**
 * @file boot.c
 * @brief DOS25 UEFI Bootloader - Core boot logic (Minimal version)
 * 
 * Simplified version to get basic functionality working first.
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#include "../../include/common.h"
#include "include/boot.h"
#endif
/**
 * @brief Check if file exists
 */
BOOLEAN file_exists(CHAR16* filename) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* file;
    
    status = BS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&loaded_image);
    if (EFI_ERROR(status)) return FALSE;
    
    status = BS->HandleProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs);
    if (EFI_ERROR(status)) return FALSE;
    
    status = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(status)) return FALSE;
    
    status = root->Open(root, &file, filename, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        root->Close(root);
        return FALSE;
    }
    
    file->Close(file);
    root->Close(root);
    return TRUE;
}




/**
 * @brief Get memory map (simplified version)
 */
boot_status_t get_memory_map_simple(void) {
    UINTN map_size = 0;
    EFI_MEMORY_DESCRIPTOR* map = NULL;
    UINTN map_key;
    UINTN desc_size;
    UINT32 desc_version;
    EFI_STATUS status;
    
    // Get memory map size
    status = BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        Print(L"[MEM ] Failed to get memory map size\r\n");
        return BOOT_ERROR;
    }
    
    Print(L"[MEM ] Memory map size: %lu bytes, %lu descriptors\r\n", 
          map_size, map_size / desc_size);
    
    return BOOT_SUCCESS;
}

/**
 * @brief Get memory map (full implementation)
 */
boot_status_t get_memory_map(boot_params_t* params) {
    UINTN map_size = 0;
    EFI_MEMORY_DESCRIPTOR* map = NULL;
    UINTN map_key;
    UINTN desc_size;
    UINT32 desc_version;
    EFI_STATUS status;
    
    // First call to get required size
    status = BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return BOOT_ERROR;
    }
    
    // Add extra space for potential changes
    map_size += 2 * desc_size;
    
    // Allocate buffer
    status = BS->AllocatePool(EfiLoaderData, map_size, (VOID**)&map);
    if (EFI_ERROR(status)) {
        return BOOT_ERROR_MEMORY;
    }
    
    // Get actual memory map
    status = BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
    if (EFI_ERROR(status)) {
        BS->FreePool(map);
        return BOOT_ERROR;
    }
    
    // Store in boot params
    params->memory_map_addr = (u64)map;
    params->memory_map_size = map_size;
    params->memory_map_desc_size = desc_size;
    params->memory_map_desc_version = desc_version;
    params->memory_map_key = map_key;
    
    return BOOT_SUCCESS;
}

/**
 * @brief Show boot menu
 */
boot_status_t show_boot_menu(boot_params_t* params) {
    EFI_INPUT_KEY key;
    UINTN index;
    EFI_EVENT events[1];
    
    Print(L"\r\n=== DOS25 Boot Menu ===\r\n");
    Print(L"1. Boot DOS25 Kernel\r\n");
    Print(L"2. UEFI Shell\r\n");
    Print(L"3. System Information\r\n");
    Print(L"4. Shutdown\r\n");
    Print(L"\r\nSelect option (1-4): ");
    
    // Wait for key press
    events[0] = ST->ConIn->WaitForKey;
    BS->WaitForEvent(1, events, &index);
    ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    
    Print(L"%c\r\n\r\n", key.UnicodeChar);
    
    switch (key.UnicodeChar) {
        case L'1':
            Print(L"[MENU] Booting DOS25 Kernel...\r\n");
            break;
        case L'2':
            Print(L"[MENU] UEFI Shell not available\r\n");
            break;
        case L'3':
            Print(L"[MENU] System: DOS25 UEFI Bootloader v%u\r\n", DOS25_RUNTIME_VERSION);
            Print(L"[MENU] Memory: %lu MB available\r\n", params->shared_buffer_size / (1024*1024));
            break;
        case L'4':
            Print(L"[MENU] Shutting down...\r\n");
            shutdown_system();
            break;
        default:
            Print(L"[MENU] Invalid option, continuing boot...\r\n");
            break;
    }
    
    return BOOT_SUCCESS;
}

/**
 * @brief Prepare kernel handoff
 */
boot_status_t prepare_kernel_handoff(boot_params_t* params) {
    Print(L"[PREP] Preparing kernel handoff...\r\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Shutdown system
 */
void shutdown_system(void) {
    Print(L"[SYS ] Shutting down system...\r\n");
    ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}

/**
 * @brief Load kernel (simplified version)
 */
boot_status_t load_kernel(boot_params_t* params) {
    if (!params) return BOOT_ERROR;
    
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS kernel_addr = 0x100000; // 1MB
    UINTN kernel_size = 1024 * 1024; // 1MB
    UINTN pages = EFI_SIZE_TO_PAGES(kernel_size);
    
    Print(L"[KERN] Starting kernel loading process...\r\n");
    
    // Try to load kernel.elf from root directory
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    status = BS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&loaded_image);
    if (!EFI_ERROR(status)) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
        status = BS->HandleProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs);
        if (!EFI_ERROR(status)) {
            EFI_FILE_PROTOCOL* root;
            status = fs->OpenVolume(fs, &root);
            if (!EFI_ERROR(status)) {
                EFI_FILE_PROTOCOL* kernel_file;
                status = root->Open(root, &kernel_file, L"kernel.elf", EFI_FILE_MODE_READ, 0);
                if (!EFI_ERROR(status)) {
                    Print(L"[KERN] Found kernel.elf\r\n");
                    kernel_file->Close(kernel_file);
                } else {
                    Print(L"[KERN] kernel.elf not found, using placeholder\r\n");
                }
                root->Close(root);
            }
        }
    }
    
    // Allocate memory for kernel
    status = BS->AllocatePages(
        AllocateAddress,
        EfiLoaderCode,
        pages,
        &kernel_addr
    );
    
    if (EFI_ERROR(status)) {
        // Try any address if fixed address fails
        kernel_addr = 0;
        status = BS->AllocatePages(
            AllocateAnyPages,
            EfiLoaderCode,
            pages,
            &kernel_addr
        );
    }
    
    if (EFI_ERROR(status)) {
        Print(L"[KERN] Failed to allocate kernel memory\r\n");
        return BOOT_ERROR_MEMORY;
    }
    
    // Store kernel information (but don't generate executable code)
    params->kernel_entry = kernel_addr;
    params->kernel_size = kernel_size;
    params->kernel_base = kernel_addr;
    
    // Clear the allocated memory (don't put any executable code)
    UINT8* kernel_mem = (UINT8*)kernel_addr;
    SetMem(kernel_mem, kernel_size, 0x00); // Clear to zeros
    
    Print(L"[KERN] Kernel memory allocated at 0x%lx, size: 0x%lx\r\n", 
          params->kernel_entry, kernel_size);
    Print(L"[KERN] Memory cleared (demo mode - no executable code)\r\n");
    
    return BOOT_SUCCESS;
}

/**
 * @brief Main boot sequence
 */
boot_status_t boot_params_init(boot_params_t* params) {
    boot_status_t status;
    
    Print(L"[BOOT] Initializing DOS25 Runtime Architecture...\r\n");
    
    // Initialize boot params structure
    params->signature = BOOT_PARAMS_SIGNATURE;
    params->version = DOS25_RUNTIME_VERSION;
    params->size = sizeof(boot_params_t);
    params->daemon_process_id = 1;  // Reserved PID for daemon
    
    // Step 1: Get memory map
    Print(L"[BOOT] Step 1: Getting memory map...\r\n");
    status = get_memory_map_simple();
    if (status != BOOT_SUCCESS) {
        Print(L"[BOOT] Memory map failed\r\n");
        return status;
    }
    Print(L"[BOOT] Memory map OK\r\n");
    
    // Step 2: Setup shared memory for IPC
    Print(L"[BOOT] Step 2: Setting up shared memory...\r\n");
    status = setup_shared_memory(params);
    if (status != BOOT_SUCCESS) {
        Print(L"[BOOT] Shared memory setup failed\r\n");
        return status;
    }
    Print(L"[BOOT] Shared memory OK\r\n");
    
    // Step 3: Preserve UEFI Runtime Services
    Print(L"[BOOT] Step 3: Preserving Runtime Services...\r\n");
    status = preserve_runtime_services(params);
    if (status != BOOT_SUCCESS) {
        Print(L"[BOOT] Runtime Services preservation failed\r\n");
        return status;
    }
    Print(L"[BOOT] Runtime Services OK\r\n");
    
    // Step 4: Initialize graphics
    Print(L"[BOOT] Step 4: Initializing graphics...\r\n");
    status = graphics_init(params);
    if (status != BOOT_SUCCESS) {
        Print(L"[BOOT] Graphics failed, continuing...\r\n");
    } else {
        Print(L"[BOOT] Graphics OK\r\n");
    }
    
    // Step 5: Load kernel
    Print(L"[BOOT] Step 5: Loading kernel...\r\n");
    status = load_kernel(params);
    if (status != BOOT_SUCCESS) {
        Print(L"[BOOT] Failed to load kernel\r\n");
        return status;
    }
    Print(L"[BOOT] Kernel loaded OK\r\n");
    
    Print(L"[BOOT] DOS25 Runtime Architecture initialized!\r\n");
    Print(L"[BOOT] Shared Memory: 0x%lx (%lu bytes)\r\n", 
          params->shared_buffer, params->shared_buffer_size);
    Print(L"[BOOT] Runtime Services: 0x%lx\r\n", params->rt_services_phys);
    
    return BOOT_SUCCESS;
}

/**
 * @brief Jump to kernel and start runtime daemon
 */
boot_status_t jump_to_kernel(boot_params_t* params) {
    Print(L"[KERN] DOS25 Runtime Architecture Demo Mode\r\n");
    Print(L"[KERN] Kernel would be loaded at: 0x%lx\r\n", params->kernel_entry);
    Print(L"[KERN] Shared memory: 0x%lx (%lu bytes)\r\n", 
          params->shared_buffer, params->shared_buffer_size);
    Print(L"[KERN] Runtime services: 0x%lx\r\n", params->rt_services_phys);
    
    // Display runtime architecture summary
    Print(L"\r\n=== DOS25 Runtime Architecture Summary ===\r\n");
    Print(L"Signature: 0x%lx\r\n", params->signature);
    Print(L"Version: %u\r\n", params->version);
    Print(L"Daemon PID: %u\r\n", params->daemon_process_id);
    Print(L"IPC Queues: Kernel<->Daemon\r\n");
    Print(L"Message capacity: %u messages\r\n", IPC_QUEUE_SIZE);
    Print(L"Max message size: %u bytes\r\n", IPC_MAX_DATA_SIZE);
    Print(L"==========================================\r\n\r\n");
    
    // Test IPC system
    Print(L"[KERN] Testing IPC system...\r\n");
    
    // Create a test message
    ipc_message_t test_msg = {0};
    test_msg.type = IPC_MSG_GET_TIME;
    test_msg.source = 0; // Kernel PID
    test_msg.target = 1; // Daemon PID
    test_msg.size = 0;
    test_msg.sequence = 1;
    
    // Send test message
    boot_status_t status = ipc_send_message(
        &params->shared_header->kernel_to_daemon, 
        &test_msg
    );
    
    if (status == BOOT_SUCCESS) {
        Print(L"[KERN] Test message sent successfully\r\n");
    } else {
        Print(L"[KERN] Failed to send test message\r\n");
    }
    
    Print(L"[KERN] Starting daemon message loop (demo mode)...\r\n");
    
    // Start daemon in demo mode (don't jump to invalid kernel)
    start_runtime_daemon(params);
    
    Print(L"\r\n[KERN] DOS25 bootloader demo completed successfully!\r\n");
    Print(L"[KERN] Kernel code prepared at 0x%lx (demo mode - not jumping)\r\n", params->kernel_entry);
    Print(L"[KERN] In production, would call ExitBootServices and jump to kernel\r\n");
    Print(L"[KERN] Press any key to exit...\r\n");
    
    // Wait for keypress before exiting
    EFI_INPUT_KEY key;
    UINTN index;
    EFI_EVENT events[1];
    events[0] = ST->ConIn->WaitForKey;
    BS->WaitForEvent(1, events, &index);
    ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    
    return BOOT_SUCCESS;
}