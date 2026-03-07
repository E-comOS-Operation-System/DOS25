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
 * @file memory.c
 * @brief DOS25 UEFI Bootloader - Memory management
 *
 * Uses GNU-EFI standard functions for all operations. Only implements
 * functionality not provided by GNU-EFI.
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#include "../../include/common.h"
#include "include/boot.h"
#endif
// Global memory map information
static EFI_MEMORY_DESCRIPTOR* gMemoryMap = NULL;
static UINTN gMemoryMapSize = 0;
static UINTN gMapKey = 0;
static UINTN gDescriptorSize = 0;
static UINT32 gDescriptorVersion = 0;

/**
 * @brief Retrieve the complete memory map from UEFI
 *
 * Uses GNU-EFI standard functions for memory map operations.
 */
boot_status_t memory_get_map(void) {
    EFI_STATUS status;

    Print(L"[MEM] Getting memory map size...\r\n");

    // First call: Get the required buffer size using GNU-EFI standard function
    status = gBS->GetMemoryMap(
        &gMemoryMapSize,
        NULL,
        &gMapKey,
        &gDescriptorSize,
        &gDescriptorVersion
    );

    if (status != EFI_BUFFER_TOO_SMALL) {
        Print(L"[MEM] ERROR: Failed to get memory map size - %r\r\n", status);
        return BOOT_ERROR_MEMORY;
    }

    // Add safety margin as per UEFI specification
    gMemoryMapSize += 2 * gDescriptorSize;

    Print(L"[MEM] Allocating memory map buffer...\r\n");

    // Use GNU-EFI's standard memory allocation
    status = gBS->AllocatePool(
        EfiLoaderData,
        gMemoryMapSize,
        (void**)&gMemoryMap
    );

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Failed to allocate memory for map - %r\r\n", status);
        return BOOT_ERROR_MEMORY;
    }

    Print(L"[MEM] Retrieving full memory map...\r\n");

    // Get the actual memory map using GNU-EFI standard function
    status = gBS->GetMemoryMap(
        &gMemoryMapSize,
        gMemoryMap,
        &gMapKey,
        &gDescriptorSize,
        &gDescriptorVersion
    );

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Failed to retrieve memory map - %r\r\n", status);
        gBS->FreePool(gMemoryMap);
        gMemoryMap = NULL;
        return BOOT_ERROR_MEMORY;
    }

    Print(L"[MEM] Memory map retrieved successfully - %lu entries\r\n", 
          gMemoryMapSize / gDescriptorSize);
    return BOOT_SUCCESS;
}

/**
 * @brief Populate memory information in boot parameters
 */
boot_status_t memory_fill_boot_params(boot_params_t* params) {
    if (!gMemoryMap) {
        return BOOT_ERROR_MEMORY;
    }

    params->memory_map_addr = (u64)gMemoryMap;
    params->memory_map_size = gMemoryMapSize;
    params->memory_map_desc_size = gDescriptorSize;
    params->memory_map_desc_version = gDescriptorVersion;
    params->memory_map_key = gMapKey;

    return BOOT_SUCCESS;
}

/**
 * @brief Allocate memory pages using GNU-EFI standard function
 */
void* memory_allocate_pages(UINTN num_pages, EFI_MEMORY_TYPE memory_type) {
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS address;

    status = gBS->AllocatePages(
        AllocateAnyPages,
        memory_type,
        num_pages,
        &address
    );

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Page allocation failed - %r\r\n", status);
        return NULL;
    }

    return (void*)(UINTN)address;
}

/**
 * @brief Free memory pages using GNU-EFI standard function
 */
boot_status_t memory_free_pages(void* address, UINTN num_pages) {
    EFI_STATUS status;

    status = gBS->FreePages(
        (EFI_PHYSICAL_ADDRESS)(UINTN)address,
        num_pages
    );

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Page free failed - %r\r\n", status);
        return BOOT_ERROR_MEMORY;
    }

    return BOOT_SUCCESS;
}

/**
 * @brief Allocate pool memory using GNU-EFI standard function
 */
void* memory_allocate_pool(UINTN size, EFI_MEMORY_TYPE memory_type) {
    EFI_STATUS status;
    void* buffer = NULL;

    status = gBS->AllocatePool(memory_type, size, &buffer);

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Pool allocation failed - %r\r\n", status);
        return NULL;
    }

    return buffer;
}

/**
 * @brief Free pool memory using GNU-EFI standard function
 */
boot_status_t memory_free_pool(void* buffer) {
    EFI_STATUS status;

    status = gBS->FreePool(buffer);

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Pool free failed - %r\r\n", status);
        return BOOT_ERROR_MEMORY;
    }

    return BOOT_SUCCESS;
}

/**
 * @brief Exit UEFI boot services using GNU-EFI standard function
 */
boot_status_t memory_exit_boot_services(void) {
    EFI_STATUS status;

    Print(L"[MEM] Exiting boot services...\r\n");

    status = gBS->ExitBootServices(gImageHandle, gMapKey);

    if (EFI_ERROR(status)) {
        Print(L"[MEM] ERROR: Failed to exit boot services - %r\r\n", status);
        
        // If first attempt fails, try to get updated memory map
        if (gMemoryMap) {
            gBS->FreePool(gMemoryMap);
            gMemoryMap = NULL;
        }
        
        // Get updated memory map and retry
        if (memory_get_map() != BOOT_SUCCESS) {
            Print(L"[MEM] ERROR: Cannot get updated memory map\r\n");
            return BOOT_ERROR;
        }
        
        status = gBS->ExitBootServices(gImageHandle, gMapKey);
        
        if (EFI_ERROR(status)) {
            Print(L"[MEM] ERROR: Second attempt to exit boot services failed - %r\r\n", status);
            return BOOT_ERROR;
        }
    }

    Print(L"[MEM] Boot services exited successfully\r\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Print memory map summary using GNU-EFI Print with formatting
 */
void memory_print_summary(void) {
    if (!gMemoryMap) {
        Print(L"[MEM] No memory map available\r\n");
        return;
    }

    UINTN num_entries = gMemoryMapSize / gDescriptorSize;
    
    // Use GNU-EFI's Print with built-in formatting - no need for custom conversion
    Print(L"[MEM] Memory Map Summary:\r\n");
    Print(L"[MEM] Entries: %lu\r\n", num_entries);
    Print(L"[MEM] Descriptor Size: %lu bytes\r\n", gDescriptorSize);
    Print(L"[MEM] Map Key: %lu\r\n", gMapKey);
    Print(L"[MEM] Descriptor Version: %u\r\n", gDescriptorVersion);
}