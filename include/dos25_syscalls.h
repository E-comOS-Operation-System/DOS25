/**
 * @file dos25_syscalls.h
 * @brief DOS25 System Call Interface
 * 
 * Defines the system call interface for accessing DOS25 runtime services
 */

#ifndef DOS25_SYSCALLS_H
#define DOS25_SYSCALLS_H

#include "common.h"

/* System Call Numbers */
#define SYS_DOS25_RESET_SYSTEM    0x1000
#define SYS_DOS25_GET_TIME        0x1001
#define SYS_DOS25_SET_TIME        0x1002
#define SYS_DOS25_GET_VARIABLE    0x1003
#define SYS_DOS25_SET_VARIABLE    0x1004
#define SYS_DOS25_GET_STATUS      0x1005

/* Reset Types */
#define DOS25_RESET_COLD          0
#define DOS25_RESET_WARM          1
#define DOS25_RESET_SHUTDOWN      2

/* Time Structure */
typedef struct {
    u16 year;        // 1900-9999
    u8  month;       // 1-12
    u8  day;         // 1-31
    u8  hour;        // 0-23
    u8  minute;      // 0-59
    u8  second;      // 0-59
    u32 nanosecond;  // 0-999,999,999
    s16 timezone;    // -1440 to 1440 or 2047
    u8  daylight;    // Daylight savings time information
} dos25_time_t;

/* Variable Attributes */
#define DOS25_VAR_NON_VOLATILE                0x00000001
#define DOS25_VAR_BOOTSERVICE_ACCESS          0x00000002
#define DOS25_VAR_RUNTIME_ACCESS              0x00000004
#define DOS25_VAR_HARDWARE_ERROR_RECORD       0x00000008
#define DOS25_VAR_AUTHENTICATED_WRITE_ACCESS  0x00000010

/* Status Structure */
typedef struct {
    u64 messages_sent;
    u64 messages_received;
    u64 errors;
    u32 daemon_status;
    u32 queue_utilization;
} dos25_status_t;

/* Function Prototypes for Kernel/User Space */

/**
 * @brief Reset the system
 * @param reset_type Type of reset (cold, warm, shutdown)
 * @return 0 on success, negative error code on failure
 */
int dos25_reset_system(u32 reset_type);

/**
 * @brief Get current system time
 * @param time Pointer to time structure to fill
 * @return 0 on success, negative error code on failure
 */
int dos25_get_time(dos25_time_t* time);

/**
 * @brief Set system time
 * @param time Pointer to time structure with new time
 * @return 0 on success, negative error code on failure
 */
int dos25_set_time(const dos25_time_t* time);

/**
 * @brief Get UEFI variable
 * @param name Variable name (wide string)
 * @param guid Variable GUID
 * @param attributes Pointer to store variable attributes
 * @param data_size Pointer to data size (in/out)
 * @param data Pointer to data buffer
 * @return 0 on success, negative error code on failure
 */
int dos25_get_variable(const u16* name, const u8* guid, u32* attributes, 
                       u64* data_size, void* data);

/**
 * @brief Set UEFI variable
 * @param name Variable name (wide string)
 * @param guid Variable GUID
 * @param attributes Variable attributes
 * @param data_size Data size
 * @param data Pointer to data
 * @return 0 on success, negative error code on failure
 */
int dos25_set_variable(const u16* name, const u8* guid, u32 attributes,
                       u64 data_size, const void* data);

/**
 * @brief Get DOS25 daemon status
 * @param status Pointer to status structure to fill
 * @return 0 on success, negative error code on failure
 */
int dos25_get_status(dos25_status_t* status);

/* Error Codes */
#define DOS25_SUCCESS           0
#define DOS25_ERROR_INVALID     -1
#define DOS25_ERROR_NOT_FOUND   -2
#define DOS25_ERROR_NO_MEMORY   -3
#define DOS25_ERROR_TIMEOUT     -4
#define DOS25_ERROR_IO          -5
#define DOS25_ERROR_UNSUPPORTED -6

#endif /* DOS25_SYSCALLS_H */