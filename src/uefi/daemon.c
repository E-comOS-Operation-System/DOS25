/**
 * @file daemon.c
 * @brief DOS25 Runtime Daemon
 * 
 * The runtime daemon that continues running after kernel handoff,
 * providing UEFI Runtime Services access via IPC
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#include "../../include/common.h"
#include "include/boot.h"
#endif
/* Global daemon state */
static shared_memory_header_t* g_shared_memory = NULL;
static EFI_RUNTIME_SERVICES* g_runtime_services = NULL;
static BOOLEAN g_daemon_running = FALSE;

/**
 * @brief Process reset system request
 */
static boot_status_t handle_reset_system(const ipc_message_t* request, ipc_message_t* response) {
    u32 reset_type;
    
    if (request->size < sizeof(u32)) {
        response->type = request->type;
        response->size = 0;
        return BOOT_ERROR;
    }
    
    CopyMem(&reset_type, request->data, sizeof(u32));
    
    Print(L"[DAEMON] Reset system request: type=%u\r\n", reset_type);
    
    // Prepare response
    response->type = request->type;
    response->source = 1; // Daemon PID
    response->target = request->source;
    response->size = sizeof(u32);
    
    // Call UEFI Runtime Service
    EFI_STATUS status = g_runtime_services->ResetSystem(
        (EFI_RESET_TYPE)reset_type, 
        EFI_SUCCESS, 
        0, 
        NULL
    );
    
    // Store result (though we shouldn't reach here for reset)
    CopyMem(response->data, &status, sizeof(u32));
    
    return BOOT_SUCCESS;
}

/**
 * @brief Process get time request
 */
static boot_status_t handle_get_time(const ipc_message_t* request, ipc_message_t* response) {
    EFI_TIME time;
    EFI_TIME_CAPABILITIES capabilities;
    
    Print(L"[DAEMON] Get time request\r\n");
    
    // Call UEFI Runtime Service
    EFI_STATUS status = g_runtime_services->GetTime(&time, &capabilities);
    
    // Prepare response
    response->type = request->type;
    response->source = 1; // Daemon PID
    response->target = request->source;
    response->size = sizeof(EFI_STATUS) + sizeof(EFI_TIME);
    
    // Pack response data
    CopyMem(response->data, &status, sizeof(EFI_STATUS));
    if (!EFI_ERROR(status)) {
        CopyMem(response->data + sizeof(EFI_STATUS), &time, sizeof(EFI_TIME));
    }
    
    return BOOT_SUCCESS;
}

/**
 * @brief Process set time request
 */
static boot_status_t handle_set_time(const ipc_message_t* request, ipc_message_t* response) {
    EFI_TIME time;
    
    if (request->size < sizeof(EFI_TIME)) {
        response->type = request->type;
        response->size = sizeof(EFI_STATUS);
        EFI_STATUS error = EFI_INVALID_PARAMETER;
        CopyMem(response->data, &error, sizeof(EFI_STATUS));
        return BOOT_ERROR;
    }
    
    CopyMem(&time, request->data, sizeof(EFI_TIME));
    
    Print(L"[DAEMON] Set time request: %02u:%02u:%02u\r\n", 
          time.Hour, time.Minute, time.Second);
    
    // Call UEFI Runtime Service
    EFI_STATUS status = g_runtime_services->SetTime(&time);
    
    // Prepare response
    response->type = request->type;
    response->source = 1; // Daemon PID
    response->target = request->source;
    response->size = sizeof(EFI_STATUS);
    
    CopyMem(response->data, &status, sizeof(EFI_STATUS));
    
    return BOOT_SUCCESS;
}

/**
 * @brief Process get variable request
 */
static boot_status_t handle_get_variable(const ipc_message_t* request, ipc_message_t* response) {
    // Variable name is in request data
    CHAR16* var_name = (CHAR16*)request->data;
    EFI_GUID vendor_guid = {0}; // Simplified - should be in request
    UINT32 attributes;
    UINTN data_size = IPC_MAX_DATA_SIZE - sizeof(EFI_STATUS) - sizeof(UINT32);
    VOID* data_buffer = response->data + sizeof(EFI_STATUS) + sizeof(UINT32);
    
    Print(L"[DAEMON] Get variable request: %s\r\n", var_name);
    
    // Call UEFI Runtime Service
    EFI_STATUS status = g_runtime_services->GetVariable(
        var_name,
        &vendor_guid,
        &attributes,
        &data_size,
        data_buffer
    );
    
    // Prepare response
    response->type = request->type;
    response->source = 1; // Daemon PID
    response->target = request->source;
    response->size = sizeof(EFI_STATUS) + sizeof(UINT32) + data_size;
    
    // Pack response data
    CopyMem(response->data, &status, sizeof(EFI_STATUS));
    CopyMem(response->data + sizeof(EFI_STATUS), &attributes, sizeof(UINT32));
    
    return BOOT_SUCCESS;
}

/**
 * @brief Process IPC message
 */
static boot_status_t process_message(const ipc_message_t* request, ipc_message_t* response) {
    boot_status_t status = BOOT_SUCCESS;
    
    // Initialize response header
    SetMem(response, sizeof(ipc_message_t), 0);
    response->timestamp = request->timestamp; // Echo timestamp
    response->sequence = request->sequence;
    
    switch (request->type) {
        case IPC_MSG_RESET_SYSTEM:
            status = handle_reset_system(request, response);
            break;
            
        case IPC_MSG_GET_TIME:
            status = handle_get_time(request, response);
            break;
            
        case IPC_MSG_SET_TIME:
            status = handle_set_time(request, response);
            break;
            
        case IPC_MSG_GET_VARIABLE:
            status = handle_get_variable(request, response);
            break;
            
        case IPC_MSG_SHUTDOWN:
            Print(L"[DAEMON] Shutdown request received\r\n");
            g_daemon_running = FALSE;
            response->type = request->type;
            response->size = 0;
            break;
            
        default:
            Print(L"[DAEMON] Unknown message type: 0x%x\r\n", request->type);
            response->type = request->type;
            response->size = sizeof(EFI_STATUS);
            EFI_STATUS error = EFI_UNSUPPORTED;
            CopyMem(response->data, &error, sizeof(EFI_STATUS));
            status = BOOT_ERROR;
            break;
    }
    
    return status;
}

/**
 * @brief Main daemon message loop
 */
boot_status_t daemon_message_loop(shared_memory_header_t* shared_mem, EFI_RUNTIME_SERVICES* rt_services) {
    ipc_message_t request, response;
    boot_status_t status;
    UINTN message_count = 0;
    const UINTN MAX_MESSAGES = 5; // Limit for demo
    
    g_shared_memory = shared_mem;
    g_runtime_services = rt_services;
    g_daemon_running = TRUE;
    
    Print(L"[DAEMON] Starting message loop (demo mode)...\r\n");
    Print(L"[DAEMON] Shared memory: 0x%lx\r\n", (u64)shared_mem);
    Print(L"[DAEMON] Runtime services: 0x%lx\r\n", (u64)rt_services);
    Print(L"[DAEMON] Will process up to %lu messages\r\n", MAX_MESSAGES);
    
    // Safety check: ensure we don't access invalid memory
    if (!shared_mem || !rt_services) {
        Print(L"[DAEMON] ERROR: Invalid parameters\r\n");
        return BOOT_ERROR;
    }
    
    while (g_daemon_running && message_count < MAX_MESSAGES) {
        // Check for incoming messages
        status = ipc_receive_message(&shared_mem->kernel_to_daemon, &request);
        
        if (status == BOOT_SUCCESS) {
            Print(L"[DAEMON] Received message type 0x%x from PID %u\r\n", 
                  request.type, request.source);
            
            // Process the message
            status = process_message(&request, &response);
            
            // Send response
            if (status == BOOT_SUCCESS) {
                status = ipc_send_message(&shared_mem->daemon_to_kernel, &response);
                if (status == BOOT_SUCCESS) {
                    shared_mem->messages_sent++;
                    Print(L"[DAEMON] Response sent successfully\r\n");
                } else {
                    Print(L"[DAEMON] Failed to send response\r\n");
                    shared_mem->errors++;
                }
            } else {
                shared_mem->errors++;
            }
            
            shared_mem->messages_received++;
            message_count++;
        } else {
            // No messages available, break in demo mode
            Print(L"[DAEMON] No messages in queue, demo complete\r\n");
            break;
        }
    }
    
    Print(L"[DAEMON] Message loop completed safely\r\n");
    Print(L"[DAEMON] Processed %lu messages\r\n", message_count);
    Print(L"[DAEMON] Total sent: %lu, received: %lu, errors: %lu\r\n", 
          shared_mem->messages_sent, shared_mem->messages_received, shared_mem->errors);
    
    return BOOT_SUCCESS;
}

/**
 * @brief Initialize and start the runtime daemon
 */
boot_status_t start_runtime_daemon(boot_params_t* params) {
    Print(L"[DAEMON] Starting DOS25 Runtime Daemon...\r\n");
    
    if (!params->shared_header || !params->rt_services) {
        Print(L"[DAEMON] Invalid parameters\r\n");
        return BOOT_ERROR;
    }
    
    // Verify shared memory integrity
    if (params->shared_header->magic != SHARED_MEM_MAGIC) {
        Print(L"[DAEMON] Shared memory corruption detected\r\n");
        return BOOT_ERROR;
    }
    
    Print(L"[DAEMON] Shared memory verified (magic: 0x%lx)\r\n", 
          params->shared_header->magic);
    
    // Start the daemon message loop
    return daemon_message_loop(params->shared_header, params->rt_services);
}