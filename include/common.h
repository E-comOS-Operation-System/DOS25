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
 * @file common.h
 * @brief DOS25 UEFI Bootloader - Essential common definitions
 * Date: Jan 14, 2026 By Saladin5101
 * Minimal header for bootloader development. Self-contained, no external dependencies.
 */

#ifndef DOS25_COMMON_H
#define DOS25_COMMON_H

#ifndef _STDINT_H
#define _STDINT_H
#endif

#ifndef _STDDEF_H
#define _STDDEF_H
#endif

#ifdef DEBUG
    #define DEBUG_MSG(msg) Print(L"DEBUG: %a\n", msg)
    #define DEBUG_HEX(val) Print(L"DEBUG: 0x%x\n", val)
#else
    #define DEBUG_MSG(msg)
    #define DEBUG_HEX(val)
#endif

#define _STDDEF_H
#define _STDINT_H
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef void* EFI_HANDLE;
/* Basic integer types for internal use */
typedef long               int64_t;
typedef unsigned long      uint64_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef signed char        int8_t;
typedef unsigned char      uint8_t;

typedef unsigned long      uintptr_t;
typedef long               intptr_t;
typedef unsigned long      size_t;

typedef uint16_t wchar_t;  
typedef uint64_t uintptr_t;
typedef int64_t  intptr_t;
typedef uint64_t size_t;


#ifndef NULL
#define NULL ((void*)0)
#endif

/* GNU-EFI want to use  */
/* We want to use */
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64; 

typedef signed char     s8;
typedef signed short    s16;
typedef signed int      s32;
typedef signed long     s64;  

/* Boot status codes */
typedef enum {
    BOOT_SUCCESS = 0,
    BOOT_ERROR = -1,
    BOOT_ERROR_MEMORY = -2,
    BOOT_ERROR_DISK = -3,
    BOOT_ERROR_FORMAT = -4,
    BOOT_ERROR_NOT_FOUND = -5,
    BOOT_ERROR_IO = -6
} boot_status_t;
/* Def in all*/
extern EFI_SYSTEM_TABLE* gSystemTable;
extern EFI_HANDLE gImageHandle;
/* Memory constants */
#define KB 1024ULL
#define MB (1024ULL * KB)
#define GB (1024ULL * MB)
#define PAGE_SIZE 4096ULL

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

#endif /* DOS25_COMMON_H */