/**
 * @file runtime.c
 * @brief DOS25 Runtime Services Management
 * 
 * Manages UEFI Runtime Services preservation and shared memory setup
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
 * @brief Setup shared memory region for IPC
 */
boot_status_t setup_shared_memory(boot_params_t* params) {
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS buffer_addr = 0;
    UINTN pages = EFI_SIZE_TO_PAGES(SHARED_MEM_SIZE);
    
    Print(L"[RT  ] Setting up shared memory (%lu pages)...\r\n", pages);
    
    // Allocate shared memory in runtime data region
    status = BS->AllocatePages(
        AllocateAnyPages,
        EfiRuntimeServicesData,
        pages,
        &buffer_addr
    );
    
    if (EFI_ERROR(status)) {
        Print(L"[RT  ] Failed to allocate shared memory: %r\r\n", status);
        return BOOT_ERROR_MEMORY;
    }
    
    // Store shared memory information
    params->shared_buffer = buffer_addr;
    params->shared_buffer_size = SHARED_MEM_SIZE;
    params->shared_header = (shared_memory_header_t*)buffer_addr;
    
    Print(L"[RT  ] Shared memory allocated at 0x%lx\r\n", buffer_addr);
    
    // Initialize shared memory header
    return initialize_ipc_queues(params->shared_header);
}

/**
 * @brief Initialize IPC queues in shared memory
 */
boot_status_t initialize_ipc_queues(shared_memory_header_t* header) {
    Print(L"[RT  ] Initializing IPC queues...\r\n");
    
    // Clear entire shared memory
    SetMem(header, SHARED_MEM_SIZE, 0);
    
    // Initialize header
    header->magic = SHARED_MEM_MAGIC;
    header->version = DOS25_RUNTIME_VERSION;
    header->size = SHARED_MEM_SIZE;
    header->sequence = 0;
    header->status = 0;
    
    // Initialize kernel->daemon queue
    header->kernel_to_daemon.producer_index = 0;
    header->kernel_to_daemon.consumer_index = 0;
    header->kernel_to_daemon.count = 0;
    header->kernel_to_daemon.capacity = IPC_QUEUE_SIZE;
    
    // Initialize daemon->kernel queue
    header->daemon_to_kernel.producer_index = 0;
    header->daemon_to_kernel.consumer_index = 0;
    header->daemon_to_kernel.count = 0;
    header->daemon_to_kernel.capacity = IPC_QUEUE_SIZE;
    
    // Initialize statistics
    header->messages_sent = 0;
    header->messages_received = 0;
    header->errors = 0;
    
    Print(L"[RT  ] IPC queues initialized\r\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Preserve UEFI Runtime Services for post-ExitBootServices access
 */
boot_status_t preserve_runtime_services(boot_params_t* params) {
    Print(L"[RT  ] Preserving UEFI Runtime Services...\r\n");
    
    // Store runtime services pointer
    params->rt_services = ST->RuntimeServices;
    params->rt_services_phys = (u64)ST->RuntimeServices;
    
    // Store in shared memory header as well
    if (params->shared_header) {
        params->shared_header->rt_services = ST->RuntimeServices;
        params->shared_header->rt_services_phys = (u64)ST->RuntimeServices;
    }
    
    Print(L"[RT  ] Runtime Services preserved at 0x%lx\r\n", 
          params->rt_services_phys);
    
    return BOOT_SUCCESS;
}

/**
 * @brief Send IPC message to queue
 */
boot_status_t ipc_send_message(ipc_queue_t* queue, const ipc_message_t* msg) {
    if (!queue || !msg) {
        return BOOT_ERROR;
    }
    
    // Check if queue is full
    if (ipc_queue_full(queue)) {
        return BOOT_ERROR_IO;
    }
    
    // Copy message to queue
    CopyMem(&queue->messages[queue->producer_index], msg, sizeof(ipc_message_t));
    
    // Update producer index (atomic)
    queue->producer_index = (queue->producer_index + 1) % queue->capacity;
    queue->count++;
    
    return BOOT_SUCCESS;
}

/**
 * @brief Receive IPC message from queue
 */
boot_status_t ipc_receive_message(ipc_queue_t* queue, ipc_message_t* msg) {
    if (!queue || !msg) {
        return BOOT_ERROR;
    }
    
    // Check if queue is empty
    if (ipc_queue_empty(queue)) {
        return BOOT_ERROR_NOT_FOUND;
    }
    
    // Copy message from queue
    CopyMem(msg, &queue->messages[queue->consumer_index], sizeof(ipc_message_t));
    
    // Update consumer index (atomic)
    queue->consumer_index = (queue->consumer_index + 1) % queue->capacity;
    queue->count--;
    
    return BOOT_SUCCESS;
}

/**
 * @brief Check if IPC queue is empty
 */
BOOLEAN ipc_queue_empty(const ipc_queue_t* queue) {
    return queue ? (queue->count == 0) : TRUE;
}

/**
 * @brief Check if IPC queue is full
 */
BOOLEAN ipc_queue_full(const ipc_queue_t* queue) {
    return queue ? (queue->count >= queue->capacity) : TRUE;
}