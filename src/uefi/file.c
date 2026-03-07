/**
 * @file file.c
 * @brief DOS25 UEFI Bootloader - File system operations
 * 
 * Handles file loading and filesystem access in UEFI environment.
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include "../../include/common.h"
#include "include/boot.h"
#include <efi/efi.h>
#include <efi/efilib.h>
#endif
/**
 * Load a file from the filesystem
 */
EFI_STATUS load_file(CHAR16* filename, VOID** buffer, UINTN* size) {
    EFI_STATUS status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* file;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    
    // Locate the file system protocol
    status = gST->BootServices->LocateProtocol(&fs_guid, NULL, (void**)&fs);
    if (EFI_ERROR(status)) {
        return status;
    }
    
    // Open the volume
    status = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(status)) {
        return status;
    }
    
    // Open the file
    status = root->Open(root, &file, filename, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        root->Close(root);
        return status;
    }
    
    // Get file size
    EFI_FILE_INFO* file_info;
    UINTN info_size = 0;
    
    // First call to get required buffer size
    status = file->GetInfo(file, &gEfiFileInfoGuid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL) {
        file->Close(file);
        root->Close(root);
        return status;
    }
    
    // Allocate buffer for file info
    status = gST->BootServices->AllocatePool(EfiLoaderData, info_size, (void**)&file_info);
    if (EFI_ERROR(status)) {
        file->Close(file);
        root->Close(root);
        return status;
    }
    
    // Get file information
    status = file->GetInfo(file, &gEfiFileInfoGuid, &info_size, file_info);
    if (EFI_ERROR(status)) {
        gST->BootServices->FreePool(file_info);
        file->Close(file);
        root->Close(root);
        return status;
    }
    
    *size = (UINTN)file_info->FileSize;
    
    // Allocate buffer for file content
    status = gST->BootServices->AllocatePool(EfiLoaderData, *size, buffer);
    if (EFI_ERROR(status)) {
        gST->BootServices->FreePool(file_info);
        file->Close(file);
        root->Close(root);
        return status;
    }
    
    // Read file content
    status = file->Read(file, size, *buffer);
    
    // Cleanup
    gST->BootServices->FreePool(file_info);
    file->Close(file);
    root->Close(root);
    
    return status;
}

/**
 * Check if a file exists
 */
BOOLEAN file_exists(CHAR16* filename) {
    VOID* buffer;
    UINTN size;
    
    EFI_STATUS status = load_file(filename, &buffer, &size);
    
    if (!EFI_ERROR(status)) {
        gST->BootServices->FreePool(buffer);
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Simple file operations for bootloader
 */
boot_status_t init_file_system(boot_params_t* params) {
    // Initialize file system - this is a placeholder
    // In a real implementation, you'd set up file system protocols here
    return BOOT_SUCCESS;
}