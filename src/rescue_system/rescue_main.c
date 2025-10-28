// DOS25 Rescue System Main

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned long size_t;

// DSF header structure
typedef struct {
    char signature[4];      // "DSF\0"
    uint32_t version;
    uint32_t total_size;
    uint32_t boot_offset;
    uint32_t boot_size;
    uint32_t kernel_offset;
    uint32_t kernel_size;
    uint32_t checksum;
} dsf_header_t;

// Simple console output
void print(const char* str) {
    while (*str) {
        char c = *str;
        __asm__ volatile (
            "movb $0x0E, %%ah\n"
            "movb %0, %%al\n"
            "int $0x10"
            : : "r" (c) : "eax"
        );
        str++;
    }
}

// Read disk sector
int read_sector(uint32_t lba, void* buffer) {
    uint16_t result;
    uint16_t buf_ptr = (uint16_t)(unsigned long)buffer;
    uint8_t sector = (uint8_t)lba;
    __asm__ volatile (
        "movb $0x02, %%ah\n"
        "movb $1, %%al\n"
        "movb $0, %%ch\n"
        "movb %2, %%cl\n"
        "movb $0, %%dh\n"
        "movb $0x80, %%dl\n"
        "movw %1, %%bx\n"
        "int $0x13\n"
        "movw %%ax, %0"
        : "=r" (result)
        : "r" (buf_ptr), "r" (sector)
        : "eax", "ebx", "ecx", "edx"
    );
    return !(result & 0x100);
}

// Write disk sector
int write_sector(uint32_t lba, const void* buffer) {
    uint16_t result;
    uint16_t buf_ptr = (uint16_t)(unsigned long)buffer;
    uint8_t sector = (uint8_t)lba;
    __asm__ volatile (
        "movb $0x03, %%ah\n"
        "movb $1, %%al\n"
        "movb $0, %%ch\n"
        "movb %2, %%cl\n"
        "movb $0, %%dh\n"
        "movb $0x80, %%dl\n"
        "movw %1, %%bx\n"
        "int $0x13\n"
        "movw %%ax, %0"
        : "=r" (result)
        : "r" (buf_ptr), "r" (sector)
        : "eax", "ebx", "ecx", "edx"
    );
    return !(result & 0x100);
}

// Calculate checksum
uint32_t checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
    }
    return sum;
}

// Install system from DSF
int install_dsf(uint32_t dsf_sector) {
    uint8_t buffer[512];
    
    print("Reading DSF header...\n");
    if (!read_sector(dsf_sector, buffer)) {
        print("ERROR: Cannot read DSF file\n");
        return 0;
    }
    
    dsf_header_t* header = (dsf_header_t*)buffer;
    
    // Validate signature
    if (header->signature[0] != 'D' || header->signature[1] != 'S' || 
        header->signature[2] != 'F') {
        print("ERROR: Invalid DSF signature\n");
        return 0;
    }
    
    print("Installing boot sector...\n");
    if (!read_sector(dsf_sector + header->boot_offset / 512, buffer)) {
        print("ERROR: Cannot read boot sector\n");
        return 0;
    }
    
    if (!write_sector(2, buffer)) {  // Write to sector 2 (original bootsect.s location)
        print("ERROR: Cannot write boot sector\n");
        return 0;
    }
    
    print("Installing kernel...\n");
    uint32_t kernel_sectors = (header->kernel_size + 511) / 512;
    for (uint32_t i = 0; i < kernel_sectors; i++) {
        if (!read_sector(dsf_sector + header->kernel_offset / 512 + i, buffer)) {
            print("ERROR: Cannot read kernel\n");
            return 0;
        }
        if (!write_sector(3 + i, buffer)) {
            print("ERROR: Cannot write kernel\n");
            return 0;
        }
    }
    
    print("Installation complete!\n");
    return 1;
}

// Rescue main entry
void rescue_main() {
    print("DOS25 Rescue System\n");
    print("===================\n\n");
    print("Commands:\n");
    print("  install - Install system from DSF\n");
    print("  reboot  - Reboot system\n\n");
    
    // Simple command loop (simplified for minimal implementation)
    print("rescue> ");
    
    // For minimal implementation, assume DSF is at sector 200
    print("install\n");
    if (install_dsf(200)) {
        print("\nSystem restored. Type 'reboot' to restart.\n");
    } else {
        print("\nInstallation failed!\n");
    }
    
    // Wait for reboot command
    while (1) {
        __asm__ volatile ("hlt");
    }
}
