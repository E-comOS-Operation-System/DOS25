// dos25-disk - DSF file management tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

uint32_t calc_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
    }
    return sum;
}

int create_dsf(const char* output, const char* boot_file, const char* kernel_file) {
    FILE* out = fopen(output, "wb");
    if (!out) {
        fprintf(stderr, "ERROR: Cannot create %s\n", output);
        return 1;
    }
    
    dsf_header_t header = {0};
    header.signature[0] = 'D';
    header.signature[1] = 'S';
    header.signature[2] = 'F';
    header.signature[3] = '\0';
    header.version = 1;
    header.boot_offset = sizeof(dsf_header_t);
    
    // Read boot sector
    FILE* boot = fopen(boot_file, "rb");
    if (!boot) {
        fprintf(stderr, "ERROR: Cannot open %s\n", boot_file);
        fclose(out);
        return 1;
    }
    fseek(boot, 0, SEEK_END);
    header.boot_size = ftell(boot);
    fseek(boot, 0, SEEK_SET);
    
    // Read kernel
    FILE* kernel = fopen(kernel_file, "rb");
    if (!kernel) {
        fprintf(stderr, "ERROR: Cannot open %s\n", kernel_file);
        fclose(boot);
        fclose(out);
        return 1;
    }
    fseek(kernel, 0, SEEK_END);
    header.kernel_size = ftell(kernel);
    fseek(kernel, 0, SEEK_SET);
    
    header.kernel_offset = header.boot_offset + header.boot_size;
    header.total_size = header.kernel_offset + header.kernel_size;
    
    // Write header
    fwrite(&header, sizeof(header), 1, out);
    
    // Write boot sector
    uint8_t* boot_data = malloc(header.boot_size);
    fread(boot_data, 1, header.boot_size, boot);
    fwrite(boot_data, 1, header.boot_size, out);
    free(boot_data);
    fclose(boot);
    
    // Write kernel
    uint8_t* kernel_data = malloc(header.kernel_size);
    fread(kernel_data, 1, header.kernel_size, kernel);
    fwrite(kernel_data, 1, header.kernel_size, out);
    free(kernel_data);
    fclose(kernel);
    
    fclose(out);
    printf("DSF file created: %s\n", output);
    return 0;
}

int verify_dsf(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open %s\n", filename);
        return 1;
    }
    
    dsf_header_t header;
    fread(&header, sizeof(header), 1, f);
    
    if (header.signature[0] != 'D' || header.signature[1] != 'S' || 
        header.signature[2] != 'F') {
        fprintf(stderr, "ERROR: Invalid DSF signature\n");
        fclose(f);
        return 1;
    }
    
    printf("DSF Version: %u\n", header.version);
    printf("Total Size: %u bytes\n", header.total_size);
    printf("Boot Size: %u bytes\n", header.boot_size);
    printf("Kernel Size: %u bytes\n", header.kernel_size);
    
    fclose(f);
    return 0;
}

int install_dsf(const char* filename, const char* device) {
    printf("Installing %s to %s...\n", filename, device);
    
    FILE* dsf = fopen(filename, "rb");
    if (!dsf) {
        fprintf(stderr, "ERROR: Cannot open %s\n", filename);
        return 1;
    }
    
    FILE* dev = fopen(device, "r+b");
    if (!dev) {
        fprintf(stderr, "ERROR: Cannot open device %s\n", device);
        fclose(dsf);
        return 1;
    }
    
    dsf_header_t header;
    fread(&header, sizeof(header), 1, dsf);
    
    // Write boot sector to sector 2
    fseek(dsf, header.boot_offset, SEEK_SET);
    fseek(dev, 1024, SEEK_SET);  // Sector 2
    uint8_t* boot_data = malloc(header.boot_size);
    fread(boot_data, 1, header.boot_size, dsf);
    fwrite(boot_data, 1, header.boot_size, dev);
    free(boot_data);
    
    // Write kernel to sector 3+
    fseek(dsf, header.kernel_offset, SEEK_SET);
    fseek(dev, 1536, SEEK_SET);  // Sector 3
    uint8_t* kernel_data = malloc(header.kernel_size);
    fread(kernel_data, 1, header.kernel_size, dsf);
    fwrite(kernel_data, 1, header.kernel_size, dev);
    free(kernel_data);
    
    fclose(dsf);
    fclose(dev);
    
    printf("Installation complete!\n");
    return 0;
}

void print_usage() {
    printf("dos25-disk - DOS25 Disk Management Tool\n\n");
    printf("Usage:\n");
    printf("  dos25-disk create <output.dsf> <boot> <kernel>\n");
    printf("  dos25-disk verify <file.dsf>\n");
    printf("  dos25-disk install <file.dsf> [device]\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        if (argc < 5) {
            print_usage();
            return 1;
        }
        return create_dsf(argv[2], argv[3], argv[4]);
    }
    
    if (strcmp(argv[1], "verify") == 0) {
        if (argc < 3) {
            print_usage();
            return 1;
        }
        return verify_dsf(argv[2]);
    }
    
    if (strcmp(argv[1], "install") == 0) {
        if (argc < 3) {
            print_usage();
            return 1;
        }
        const char* device = argc > 3 ? argv[3] : "/dev/sda";
        return install_dsf(argv[2], device);
    }
    
    print_usage();
    return 1;
}
