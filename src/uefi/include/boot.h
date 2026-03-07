/**
 * @file boot.h
 * @brief DOS25 UEFI Bootloader - Boot parameters structure
 */

#ifndef DOS25_BOOT_H
#define DOS25_BOOT_H
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#include "../../../include/common.h"
#endif
#define BOOT_PARAMS_SIGNATURE 0x444F533235525420ULL  // "DOS25RT "
#define DOS25_RUNTIME_VERSION 1

/* IPC Message Types */
#define IPC_MSG_RESET_SYSTEM    0x01
#define IPC_MSG_GET_TIME        0x02
#define IPC_MSG_SET_TIME        0x03
#define IPC_MSG_GET_VARIABLE    0x04
#define IPC_MSG_SET_VARIABLE    0x05
#define IPC_MSG_SHUTDOWN        0x06

/* Reset Types */
#define DOS25_RESET_COLD        0
#define DOS25_RESET_WARM        1
#define DOS25_RESET_SHUTDOWN    2

/* Shared Memory Layout */
#define SHARED_MEM_MAGIC        0x444F533235534D00ULL  // "DOS25SM"
#define SHARED_MEM_SIZE         (2 * 1024 * 1024)     // 2MB
#define IPC_QUEUE_SIZE          256
#define IPC_MAX_DATA_SIZE       4096

/* Forward declarations */
typedef struct boot_params_struct boot_params_t;
typedef struct shared_memory_header shared_memory_header_t;
typedef struct ipc_message ipc_message_t;
typedef struct ipc_queue ipc_queue_t;

/* IPC Message Structure */
struct ipc_message {
    u32 type;           // Message type
    u32 source;         // Source process ID
    u32 target;         // Target process ID
    u64 timestamp;      // Timestamp
    u32 size;           // Data size
    u32 sequence;       // Sequence number
    u8 data[IPC_MAX_DATA_SIZE];  // Message data
};

/* IPC Queue Structure */
struct ipc_queue {
    volatile u32 producer_index;
    volatile u32 consumer_index;
    volatile u32 count;
    u32 capacity;
    ipc_message_t messages[IPC_QUEUE_SIZE];
};

/* Shared Memory Header */
struct shared_memory_header {
    u64 magic;              // Magic number
    u32 version;            // Version
    u32 size;               // Total size
    volatile u32 sequence;  // Global sequence counter
    volatile u32 status;    // Status flags
    
    // IPC Queues
    ipc_queue_t kernel_to_daemon;   // Kernel -> Daemon
    ipc_queue_t daemon_to_kernel;   // Daemon -> Kernel
    
    // Runtime Services Context
    EFI_RUNTIME_SERVICES* rt_services;
    u64 rt_services_phys;
    
    // Statistics
    u64 messages_sent;
    u64 messages_received;
    u64 errors;
};

/* Extended Boot Parameters Structure */
struct boot_params_struct {
    /* Basic boot information */
    u64 signature;          // "DOS25RT "
    u32 version;            // Runtime version
    u32 size;               // Structure size
    
    /* Memory information */
    u64 memory_map_size;
    u64 memory_map_desc_size;
    u64 memory_map_desc_version;
    u64 memory_map_addr;
    u64 memory_map_key;
    
    /* Framebuffer information */
    u64 framebuffer_addr;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u32 framebuffer_pitch;
    u32 framebuffer_bpp;
    
    /* Kernel information */
    u64 kernel_entry;
    u64 kernel_size;
    u64 kernel_base;
    
    /* ACPI information */
    u64 acpi_rsdp_addr;
    
    /* Boot device information */
    u64 boot_device;
    
    /* Runtime Services Extension */
    EFI_RUNTIME_SERVICES* rt_services;
    u64 rt_services_phys;           // Physical address
    
    /* Shared Memory Region */
    EFI_PHYSICAL_ADDRESS shared_buffer;
    UINTN shared_buffer_size;
    shared_memory_header_t* shared_header;
    
    /* IPC Configuration */
    u64 ipc_endpoint;
    u32 daemon_process_id;
    
    /* Runtime Daemon Information */
    u64 daemon_stack_base;
    u64 daemon_stack_size;
    u64 daemon_entry_point;
    
    /* Reserved for future expansion */
    u64 reserved[4];
};

/* Function declarations */
boot_status_t boot_params_init(boot_params_t* params);
boot_status_t get_memory_map(boot_params_t* params);
boot_status_t graphics_init(boot_params_t* params);
boot_status_t load_kernel(boot_params_t* params);
boot_status_t show_boot_menu(boot_params_t* params);
boot_status_t prepare_kernel_handoff(boot_params_t* params);
boot_status_t jump_to_kernel(boot_params_t* params);
void shutdown_system(void);
BOOLEAN file_exists(CHAR16* filename);
EFI_STATUS load_file(CHAR16* filename, VOID** buffer, UINTN* size);

/* Runtime Services Functions */
boot_status_t setup_shared_memory(boot_params_t* params);
boot_status_t initialize_ipc_queues(shared_memory_header_t* header);
boot_status_t preserve_runtime_services(boot_params_t* params);

/* IPC Functions */
boot_status_t ipc_send_message(ipc_queue_t* queue, const ipc_message_t* msg);
boot_status_t ipc_receive_message(ipc_queue_t* queue, ipc_message_t* msg);
BOOLEAN ipc_queue_empty(const ipc_queue_t* queue);
BOOLEAN ipc_queue_full(const ipc_queue_t* queue);

/* Daemon Functions */
boot_status_t start_runtime_daemon(boot_params_t* params);
boot_status_t daemon_message_loop(shared_memory_header_t* shared_mem, EFI_RUNTIME_SERVICES* rt_services);

/* Test Functions */
boot_status_t run_ipc_tests(boot_params_t* params);

/* Daemon Functions */
boot_status_t start_runtime_daemon(boot_params_t* params);
boot_status_t daemon_message_loop(shared_memory_header_t* shared_mem, EFI_RUNTIME_SERVICES* rt_services);

/* Test Functions */
boot_status_t run_ipc_tests(boot_params_t* params);

#endif /* DOS25_BOOT_H */