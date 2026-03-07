/**
 * @file ipc_test.c
 * @brief DOS25 IPC System Test
 * 
 * Tests the IPC communication between kernel and daemon
 */
#if defined(__APPLE__) && defined(__MACH__)
#error "In macOS , GNU-EFI library is unsupported. Please build and run this code on a compatible platform (e.g., Linux or *BSD or Windows with WSL)."
#elif defined(__linux__)
#include <efi/efi.h>
#include <efi/efilib.h>
#include "../include/common.h"
#include "include/boot.h"
#endif
/**
 * @brief Test IPC message sending and receiving
 */
boot_status_t test_ipc_communication(shared_memory_header_t* shared_mem) {
    ipc_message_t msg, response;
    boot_status_t status;
    
    Print(L"[TEST] Testing IPC communication...\r\n");
    
    // Test 1: Send GET_TIME message
    Print(L"[TEST] Test 1: GET_TIME message\r\n");
    SetMem(&msg, sizeof(msg), 0);
    msg.type = IPC_MSG_GET_TIME;
    msg.source = 0; // Kernel PID
    msg.target = 1; // Daemon PID
    msg.size = 0;
    msg.sequence = 1;
    
    status = ipc_send_message(&shared_mem->kernel_to_daemon, &msg);
    if (status != BOOT_SUCCESS) {
        Print(L"[TEST] Failed to send GET_TIME message\r\n");
        return status;
    }
    Print(L"[TEST] GET_TIME message sent\r\n");
    
    // Test 2: Send RESET_SYSTEM message
    Print(L"[TEST] Test 2: RESET_SYSTEM message\r\n");
    SetMem(&msg, sizeof(msg), 0);
    msg.type = IPC_MSG_RESET_SYSTEM;
    msg.source = 0;
    msg.target = 1;
    msg.size = sizeof(u32);
    msg.sequence = 2;
    
    u32 reset_type = DOS25_RESET_SHUTDOWN;
    CopyMem(msg.data, &reset_type, sizeof(u32));
    
    status = ipc_send_message(&shared_mem->kernel_to_daemon, &msg);
    if (status != BOOT_SUCCESS) {
        Print(L"[TEST] Failed to send RESET_SYSTEM message\r\n");
        return status;
    }
    Print(L"[TEST] RESET_SYSTEM message sent\r\n");
    
    // Test 3: Queue status
    Print(L"[TEST] Queue status:\r\n");
    Print(L"[TEST]   Kernel->Daemon: %u/%u messages\r\n", 
          shared_mem->kernel_to_daemon.count,
          shared_mem->kernel_to_daemon.capacity);
    Print(L"[TEST]   Daemon->Kernel: %u/%u messages\r\n",
          shared_mem->daemon_to_kernel.count,
          shared_mem->daemon_to_kernel.capacity);
    
    // Test 4: Try to receive from empty queue
    Print(L"[TEST] Test 4: Receive from empty queue\r\n");
    status = ipc_receive_message(&shared_mem->daemon_to_kernel, &response);
    if (status == BOOT_ERROR_NOT_FOUND) {
        Print(L"[TEST] Correctly detected empty queue\r\n");
    } else {
        Print(L"[TEST] Unexpected result from empty queue: %d\r\n", status);
    }
    
    Print(L"[TEST] IPC communication test completed\r\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Test shared memory integrity
 */
boot_status_t test_shared_memory(shared_memory_header_t* shared_mem) {
    Print(L"[TEST] Testing shared memory integrity...\r\n");
    
    // Check magic number
    if (shared_mem->magic != SHARED_MEM_MAGIC) {
        Print(L"[TEST] FAIL: Invalid magic number: 0x%lx\r\n", shared_mem->magic);
        return BOOT_ERROR;
    }
    Print(L"[TEST] PASS: Magic number correct\r\n");
    
    // Check version
    if (shared_mem->version != DOS25_RUNTIME_VERSION) {
        Print(L"[TEST] FAIL: Invalid version: %u\r\n", shared_mem->version);
        return BOOT_ERROR;
    }
    Print(L"[TEST] PASS: Version correct\r\n");
    
    // Check size
    if (shared_mem->size != SHARED_MEM_SIZE) {
        Print(L"[TEST] FAIL: Invalid size: %u\r\n", shared_mem->size);
        return BOOT_ERROR;
    }
    Print(L"[TEST] PASS: Size correct\r\n");
    
    // Check queue initialization
    if (shared_mem->kernel_to_daemon.capacity != IPC_QUEUE_SIZE ||
        shared_mem->daemon_to_kernel.capacity != IPC_QUEUE_SIZE) {
        Print(L"[TEST] FAIL: Queue capacity incorrect\r\n");
        return BOOT_ERROR;
    }
    Print(L"[TEST] PASS: Queue capacity correct\r\n");
    
    Print(L"[TEST] Shared memory integrity test completed\r\n");
    return BOOT_SUCCESS;
}

/**
 * @brief Run comprehensive IPC tests
 */
boot_status_t run_ipc_tests(boot_params_t* params) {
    boot_status_t status;
    
    Print(L"\r\n=== DOS25 IPC System Tests ===\r\n");
    
    if (!params->shared_header) {
        Print(L"[TEST] ERROR: No shared memory available\r\n");
        return BOOT_ERROR;
    }
    
    // Test 1: Shared memory integrity
    status = test_shared_memory(params->shared_header);
    if (status != BOOT_SUCCESS) {
        Print(L"[TEST] Shared memory test FAILED\r\n");
        return status;
    }
    
    // Test 2: IPC communication
    status = test_ipc_communication(params->shared_header);
    if (status != BOOT_SUCCESS) {
        Print(L"[TEST] IPC communication test FAILED\r\n");
        return status;
    }
    
    // Display statistics
    Print(L"\r\n=== Test Statistics ===\r\n");
    Print(L"Messages sent: %lu\r\n", params->shared_header->messages_sent);
    Print(L"Messages received: %lu\r\n", params->shared_header->messages_received);
    Print(L"Errors: %lu\r\n", params->shared_header->errors);
    Print(L"Sequence counter: %u\r\n", params->shared_header->sequence);
    
    Print(L"\r\n=== All Tests PASSED ===\r\n");
    return BOOT_SUCCESS;
}