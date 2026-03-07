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
 * @file graphics.c
 * 
 * @brief DOS25 UEFI Bootloader - Graphics Output Protocol implementation
 * 
 * Handles graphics initialization and framebuffer setup for the bootloader.
 * Provides basic graphics capabilities for boot menu and debug output.
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include "../../include/common.h"
#include <efi/efi.h>
#include <efi/efilib.h>
#include "include/boot.h"
#endif
/* ==================== Global Variables ==================== */

static EFI_GRAPHICS_OUTPUT_PROTOCOL* gGOP = NULL;
static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gModeInfo = NULL;

/* ==================== Graphics Protocol Handling ==================== */

/**
 * @brief Locate and initialize Graphics Output Protocol
 */
boot_status_t graphics_init(boot_params_t* params) {
    EFI_STATUS status;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    
    DEBUG_MSG("Initializing Graphics Output Protocol...\n");
    
    // Locate GOP protocol
    status = gST->BootServices->LocateProtocol(
        &gop_guid,
        NULL,
        (void**)&gGOP
    );
    
    if (EFI_ERROR(status)) {
        DEBUG_MSG("GOP protocol not found\n");
        return BOOT_SUCCESS; // Graphics is optional, not fatal
    }
    
    DEBUG_MSG("GOP protocol located successfully\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Get information about available graphics modes
 */
boot_status_t graphics_get_available_modes(UINT32* mode_count, UINT32* current_mode) {
    if (!gGOP) {
        return BOOT_ERROR;
    }
    
    *mode_count = gGOP->Mode->MaxMode;
    *current_mode = gGOP->Mode->Mode;
    
    DEBUG_MSG("Available graphics modes: ");
    DEBUG_HEX(*mode_count);
    DEBUG_MSG(", Current: ");
    DEBUG_HEX(*current_mode);
    DEBUG_MSG("\n");
    
    return BOOT_SUCCESS;
}

/**
 * @brief Query specific graphics mode information
 */
boot_status_t graphics_query_mode(UINT32 mode_number, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION** info, UINTN* info_size) {
    EFI_STATUS status;
    
    if (!gGOP) {
        return BOOT_ERROR;
    }
    
    status = gGOP->QueryMode(
        gGOP,
        mode_number,
        info_size,
        info
    );
    
    if (EFI_ERROR(status)) {
        DEBUG_MSG("Failed to query mode ");
        DEBUG_HEX(mode_number);
        DEBUG_MSG("\n");
        return BOOT_ERROR;
    }
    
    return BOOT_SUCCESS;
}

/* ==================== Mode Selection ==================== */

/**
 * @brief Find the best available graphics mode
 * 
 * Prefers standard resolutions in this order:
 * 1. 1024x768
 * 2. 800x600  
 * 3. 640x480
 * 4. First available mode
 */
boot_status_t graphics_find_best_mode(UINT32* best_mode) {
    UINT32 mode_count, current_mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN info_size;
    
    if (!gGOP) {
        *best_mode = 0;
        return BOOT_SUCCESS; // No graphics, use mode 0
    }
    
    // Get available modes
    boot_status_t status = graphics_get_available_modes(&mode_count, &current_mode);
    if (status != BOOT_SUCCESS) {
        return status;
    }
    
    // Preferred modes in priority order
    struct preferred_mode {
        UINT32 width;
        UINT32 height;
    } preferences[] = {
        {1024, 768},
        {800, 600},
        {640, 480},
        {0, 0} // Fallback to first mode
    };
    
    // Search for preferred modes
    for (UINT32 p = 0; p < sizeof(preferences)/sizeof(preferences[0]); p++) {
        for (UINT32 i = 0; i < mode_count; i++) {
            status = graphics_query_mode(i, &info, &info_size);
            if (status != BOOT_SUCCESS) continue;
            
            // Check if this mode matches our preference
            if ((preferences[p].width == 0 && preferences[p].height == 0) || 
                (info->HorizontalResolution == preferences[p].width && 
                 info->VerticalResolution == preferences[p].height)) {
                
                *best_mode = i;
                DEBUG_MSG("Selected graphics mode ");
                DEBUG_HEX(i);
                DEBUG_MSG(" (");
                DEBUG_HEX(info->HorizontalResolution);
                DEBUG_MSG("x");
                DEBUG_HEX(info->VerticalResolution);
                DEBUG_MSG(")\n");
                
                return BOOT_SUCCESS;
            }
        }
    }
    
    // Fallback to current mode
    *best_mode = current_mode;
    return BOOT_SUCCESS;
}

/**
 * @brief Set graphics mode
 */
boot_status_t graphics_set_mode(UINT32 mode_number) {
    EFI_STATUS status;
    
    if (!gGOP) {
        return BOOT_SUCCESS; // No graphics, not an error
    }
    
    DEBUG_MSG("Setting graphics mode ");
    DEBUG_HEX(mode_number);
    DEBUG_MSG("\n");
    
    status = gGOP->SetMode(gGOP, mode_number);
    if (EFI_ERROR(status)) {
        DEBUG_MSG("Failed to set graphics mode\n");
        return BOOT_ERROR;
    }
    
    // Store mode information for boot parameters
    gModeInfo = gGOP->Mode->Info;
    
    DEBUG_MSG("Graphics mode set successfully: ");
    DEBUG_HEX(gModeInfo->HorizontalResolution);
    DEBUG_MSG("x");
    DEBUG_HEX(gModeInfo->VerticalResolution);
    DEBUG_MSG("\n");
    
    return BOOT_SUCCESS;
}

/* ==================== Framebuffer Operations ==================== */

/**
 * @brief Get framebuffer information for boot parameters
 */
boot_status_t graphics_get_framebuffer_info(boot_params_t* params) {
    if (!gGOP || !gModeInfo) {
        // No graphics available, set default values
        params->framebuffer_addr = 0;
        params->framebuffer_width = 0;
        params->framebuffer_height = 0;
        params->framebuffer_pitch = 0;
        params->framebuffer_bpp = 0;
        return BOOT_SUCCESS;
    }
    
    params->framebuffer_addr = (u64)gGOP->Mode->FrameBufferBase;
    params->framebuffer_width = gModeInfo->HorizontalResolution;
    params->framebuffer_height = gModeInfo->VerticalResolution;
    params->framebuffer_bpp = 32; // Assuming 32-bit color
    
    // Calculate pitch (bytes per scanline)
    params->framebuffer_pitch = params->framebuffer_width * (params->framebuffer_bpp / 8);
    
    DEBUG_MSG("Framebuffer: 0x");
    DEBUG_HEX(params->framebuffer_addr);
    DEBUG_MSG(" ");
    DEBUG_HEX(params->framebuffer_width);
    DEBUG_MSG("x");
    DEBUG_HEX(params->framebuffer_height);
    DEBUG_MSG(" (pitch: ");
    DEBUG_HEX(params->framebuffer_pitch);
    DEBUG_MSG(")\n");
    
    return BOOT_SUCCESS;
}

/**
 * @brief Basic pixel drawing function (for boot menu/debug)
 */
boot_status_t graphics_draw_pixel(UINT32 x, UINT32 y, UINT32 color) {
    if (!gGOP || !gModeInfo) {
        return BOOT_ERROR;
    }
    
    // Bounds checking
    if (x >= gModeInfo->HorizontalResolution || y >= gModeInfo->VerticalResolution) {
        return BOOT_ERROR;
    }
    
    // Calculate pixel position
    UINT8* pixel = (UINT8*)gGOP->Mode->FrameBufferBase;
    pixel += y * gModeInfo->PixelsPerScanLine * 4; // 4 bytes per pixel (32-bit)
    pixel += x * 4;
    
    // Write color (assuming 32-bit RGB format)
    *(UINT32*)pixel = color;
    
    return BOOT_SUCCESS;
}

/**
 * @brief Draw a simple rectangle (for boot menu background)
 */
boot_status_t graphics_draw_rect(UINT32 x, UINT32 y, UINT32 width, UINT32 height, UINT32 color) {
    for (UINT32 row = y; row < y + height; row++) {
        for (UINT32 col = x; col < x + width; col++) {
            boot_status_t status = graphics_draw_pixel(col, row, color);
            if (status != BOOT_SUCCESS) {
                return status;
            }
        }
    }
    return BOOT_SUCCESS;
}

/**
 * @brief Clear screen with specific color
 */
boot_status_t graphics_clear_screen(UINT32 color) {
    if (!gGOP || !gModeInfo) {
        return BOOT_ERROR;
    }
    
    return graphics_draw_rect(0, 0, 
                            gModeInfo->HorizontalResolution, 
                            gModeInfo->VerticalResolution, 
                            color);
}

/* ==================== Text Rendering (Basic) ==================== */

/**
 * @brief Simple character rendering for boot messages
 * 
 * This is a basic implementation for displaying text on graphics mode.
 * For production use, you'd want a proper font renderer.
 */
boot_status_t graphics_draw_char(UINT32 x, UINT32 y, char c, UINT32 color) {
    // Simple 8x8 font rendering (placeholder implementation)
    // In a real implementation, you'd have a proper font bitmap
    
    // For now, just draw a placeholder block
    return graphics_draw_rect(x, y, 8, 8, color);
}

/**
 * @brief Draw a string on graphics screen
 */
boot_status_t graphics_draw_string(UINT32 x, UINT32 y, const char* str, UINT32 color) {
    UINT32 current_x = x;
    
    while (*str) {
        boot_status_t status = graphics_draw_char(current_x, y, *str, color);
        if (status != BOOT_SUCCESS) {
            return status;
        }
        current_x += 8; // Advance to next character position
        str++;
    }
    
    return BOOT_SUCCESS;
}

/* ==================== Main Graphics Interface ==================== */

/**
 * @brief Complete graphics initialization sequence
 * 
 * This is the main function called from boot.c to set up graphics.
 */
boot_status_t graphics_initialize_complete(boot_params_t* params) {
    boot_status_t status;
    UINT32 best_mode;
    
    DEBUG_MSG("Starting complete graphics initialization...\n");
    
    // Step 1: Initialize GOP protocol
    status = graphics_init(params);
    if (status != BOOT_SUCCESS) {
        DEBUG_MSG("Graphics initialization failed, continuing without graphics...\n");
        return BOOT_SUCCESS; // Graphics is optional
    }
    
    // Step 2: Find the best available mode
    status = graphics_find_best_mode(&best_mode);
    if (status != BOOT_SUCCESS) {
        DEBUG_MSG("Failed to find graphics mode, using default...\n");
        best_mode = 0;
    }
    
    // Step 3: Set the graphics mode
    status = graphics_set_mode(best_mode);
    if (status != BOOT_SUCCESS) {
        DEBUG_MSG("Failed to set graphics mode, continuing without graphics...\n");
        return BOOT_SUCCESS; // Graphics is optional
    }
    
    // Step 4: Get framebuffer info for kernel
    status = graphics_get_framebuffer_info(params);
    if (status != BOOT_SUCCESS) {
        DEBUG_MSG("Failed to get framebuffer info\n");
        // Continue anyway, kernel will detect missing graphics
    }
    
    // Step 5: Clear screen and display boot message
    graphics_clear_screen(0x00000000); // Black background
    graphics_draw_string(10, 10, "DOS25 UEFI Bootloader", 0x00FFFFFF); // White text
    
    DEBUG_MSG("Graphics initialization completed successfully\n");
    
    return BOOT_SUCCESS;
}