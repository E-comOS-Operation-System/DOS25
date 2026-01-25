/**
 * @file dos25-disk.c
 * @brief DOS25 Disk Image Management Tool
 * 
 * Portable implementation using only standard C library functions.
 * No UNIX/POSIX dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* DSF file header structure */
struct dsf_header {
    char signature[4];      // "DSF\0"
    uint32_t version;       // Format version
    uint32_t total_size;    // Total file size
    uint32_t boot_offset;   // Boot sector offset
    uint32_t boot_size;     // Boot sector size
    uint32_t kernel_offset; // Kernel offset
    uint32_t kernel_size;   // Kernel size
    uint32_t checksum;      // Checksum
};

/* Tool operation modes */
typedef enum {
    MODE_CREATE,    // Create DSF file
    MODE_VERIFY,    // Verify DSF file
    MODE_INSTALL,   // Install to disk
    MODE_EXTRACT    // Extract from DSF
} tool_mode_t;

/**
 * Calculate simple checksum for data verification
 */
uint32_t calculate_checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 3) ^ data[i];
    }
    return sum;
}

/**
 * Create DSF disk image from boot and kernel files
 */
int create_dsf_image(const char* output_path, 
                     const char* boot_bin, 
                     const char* kernel_bin) {
    printf("Creating DSF image: %s\n", output_path);
    
    /* Read boot sector file */
    FILE* boot_file = fopen(boot_bin, "rb");
    if (!boot_file) {
        fprintf(stderr, "Error: Cannot open boot file %s\n", boot_bin);
        return -1;
    }
    
    /* Read kernel file */
    FILE* kernel_file = fopen(kernel_bin, "rb");
    if (!kernel_file) {
        fprintf(stderr, "Error: Cannot open kernel file %s\n", kernel_bin);
        fclose(boot_file);
        return -1;
    }
    
    /* Create DSF file header */
    struct dsf_header header = {
        .signature = {'D','S','F',0},
        .version = 1,
        .boot_offset = sizeof(struct dsf_header),
        .kernel_offset = sizeof(struct dsf_header) + 512
    };
    
    /* Calculate file sizes */
    fseek(boot_file, 0, SEEK_END);
    header.boot_size = ftell(boot_file);
    fseek(boot_file, 0, SEEK_SET);
    
    fseek(kernel_file, 0, SEEK_END);
    header.kernel_size = ftell(kernel_file);
    fseek(kernel_file, 0, SEEK_SET);
    
    header.total_size = sizeof(header) + header.boot_size + header.kernel_size;
    
    /* Create DSF output file */
    FILE* dsf_file = fopen(output_path, "wb");
    if (!dsf_file) {
        fprintf(stderr, "Error: Cannot create DSF file %s\n", output_path);
        fclose(boot_file);
        fclose(kernel_file);
        return -1;
    }
    
    /* Write file header */
    fwrite(&header, sizeof(header), 1, dsf_file);
    
    /* Write boot sector */
    uint8_t buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), boot_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dsf_file);
    }
    
    /* Write kernel data */
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), kernel_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dsf_file);
    }
    
    /* Calculate and update checksum */
    fseek(dsf_file, 0, SEEK_SET);
    uint8_t* file_data = (uint8_t*)malloc(header.total_size);
    if (!file_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(boot_file);
        fclose(kernel_file);
        fclose(dsf_file);
        return -1;
    }
    
    fread(file_data, 1, header.total_size, dsf_file);
    
    header.checksum = calculate_checksum(file_data + sizeof(uint32_t), 
                                        header.total_size - sizeof(uint32_t));
    
    /* Rewrite header with checksum */
    fseek(dsf_file, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, dsf_file);
    
    /* Cleanup */
    free(file_data);
    fclose(boot_file);
    fclose(kernel_file);
    fclose(dsf_file);
    
    printf("DSF image created successfully: %s (Size: %u bytes)\n", 
           output_path, header.total_size);
    return 0;
}

/**
 * Verify DSF file integrity and structure
 */
int verify_dsf_image(const char* dsf_path) {
    printf("Verifying DSF image: %s\n", dsf_path);
    
    FILE* file = fopen(dsf_path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open DSF file %s\n", dsf_path);
        return -1;
    }
    
    struct dsf_header header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fprintf(stderr, "Error: Cannot read DSF header\n");
        fclose(file);
        return -1;
    }
    
    /* Verify signature */
    if (memcmp(header.signature, "DSF", 3) != 0) {
        fprintf(stderr, "Error: Invalid DSF signature\n");
        fclose(file);
        return -1;
    }
    
    /* Verify file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size != (long)header.total_size) {
        fprintf(stderr, "Error: File size mismatch (expected: %u, actual: %ld)\n",
                header.total_size, file_size);
        fclose(file);
        return -1;
    }
    
    /* Verify checksum */
    fseek(file, 0, SEEK_SET);
    uint8_t* file_data = (uint8_t*)malloc(header.total_size);
    if (!file_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }
    
    fread(file_data, 1, header.total_size, file);
    
    uint32_t calculated_csum = calculate_checksum(file_data + sizeof(uint32_t),
                                                 header.total_size - sizeof(uint32_t));
    
    if (calculated_csum != header.checksum) {
        fprintf(stderr, "Error: Checksum mismatch (expected: %08X, calculated: %08X)\n",
                header.checksum, calculated_csum);
        free(file_data);
        fclose(file);
        return -1;
    }
    
    printf("DSF verification passed:\n");
    printf("  Version: %u\n", header.version);
    printf("  Total size: %u bytes\n", header.total_size);
    printf("  Boot sector: %u bytes at offset 0x%X\n", header.boot_size, header.boot_offset);
    printf("  Kernel: %u bytes at offset 0x%X\n", header.kernel_size, header.kernel_offset);
    printf("  Checksum: 0x%08X (valid)\n", header.checksum);
    
    free(file_data);
    fclose(file);
    return 0;
}

/**
 * Extract components from DSF image
 */
int extract_dsf_image(const char* dsf_path, 
                      const char* boot_output, 
                      const char* kernel_output) {
    printf("Extracting components from DSF image: %s\n", dsf_path);
    
    FILE* dsf_file = fopen(dsf_path, "rb");
    if (!dsf_file) {
        fprintf(stderr, "Error: Cannot open DSF file\n");
        return -1;
    }
    
    struct dsf_header header;
    if (fread(&header, sizeof(header), 1, dsf_file) != 1) {
        fprintf(stderr, "Error: Cannot read DSF header\n");
        fclose(dsf_file);
        return -1;
    }
    
    /* Extract boot sector */
    if (boot_output && boot_output[0] != '\0') {
        FILE* boot_file = fopen(boot_output, "wb");
        if (!boot_file) {
            fprintf(stderr, "Error: Cannot create boot output file\n");
            fclose(dsf_file);
            return -1;
        }
        
        fseek(dsf_file, header.boot_offset, SEEK_SET);
        
        uint8_t* boot_data = (uint8_t*)malloc(header.boot_size);
        if (!boot_data) {
            fprintf(stderr, "Error: Memory allocation failed for boot data\n");
            fclose(boot_file);
            fclose(dsf_file);
            return -1;
        }
        
        fread(boot_data, 1, header.boot_size, dsf_file);
        fwrite(boot_data, 1, header.boot_size, boot_file);
        
        free(boot_data);
        fclose(boot_file);
        printf("Boot sector extracted to: %s (%u bytes)\n", 
               boot_output, header.boot_size);
    }
    
    /* Extract kernel */
    if (kernel_output && kernel_output[0] != '\0') {
        FILE* kernel_file = fopen(kernel_output, "wb");
        if (!kernel_file) {
            fprintf(stderr, "Error: Cannot create kernel output file\n");
            fclose(dsf_file);
            return -1;
        }
        
        fseek(dsf_file, header.kernel_offset, SEEK_SET);
        
        uint8_t* kernel_data = (uint8_t*)malloc(header.kernel_size);
        if (!kernel_data) {
            fprintf(stderr, "Error: Memory allocation failed for kernel data\n");
            fclose(kernel_file);
            fclose(dsf_file);
            return -1;
        }
        
        fread(kernel_data, 1, header.kernel_size, dsf_file);
        fwrite(kernel_data, 1, header.kernel_size, kernel_file);
        
        free(kernel_data);
        fclose(kernel_file);
        printf("Kernel extracted to: %s (%u bytes)\n", 
               kernel_output, header.kernel_size);
    }
    
    fclose(dsf_file);
    return 0;
}

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
    printf("DOS25 Disk Image Management Tool (Portable)\n");
    printf("Usage: %s <command> [options]\n\n", program_name);
    printf("Commands:\n");
    printf("  create <output.dsf> <boot.bin> <kernel.bin>  Create new DSF image\n");
    printf("  verify <input.dsf>                          Verify DSF image integrity\n");
    printf("  extract <input.dsf> [boot.out] [kernel.out] Extract components\n");
    printf("\nExamples:\n");
    printf("  %s create system.dsf boot.bin kernel.bin\n", program_name);
    printf("  %s verify system.dsf\n", program_name);
    printf("  %s extract system.dsf boot.bin kernel.elf\n", program_name);
}

/**
 * Main function - command line interface
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "create") == 0 && argc == 5) {
        return create_dsf_image(argv[2], argv[3], argv[4]);
    }
    else if (strcmp(command, "verify") == 0 && argc == 3) {
        return verify_dsf_image(argv[2]);
    }
    else if (strcmp(command, "extract") == 0 && argc >= 3) {
        const char* boot_out = (argc > 3) ? argv[3] : NULL;
        const char* kernel_out = (argc > 4) ? argv[4] : NULL;
        return extract_dsf_image(argv[2], boot_out, kernel_out);
    }
    else {
        fprintf(stderr, "Error: Invalid command or arguments\n");
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}