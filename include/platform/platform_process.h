#ifndef PLATFORM_PROCESS_H
#define PLATFORM_PROCESS_H

// =============================================================================
// CROSS-PLATFORM PROCESS MANAGEMENT ABSTRACTION
// =============================================================================
// Abstracts process creation, management, and inter-process communication

#include "core/bongocat.h"
#include "utils/error.h"

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE process_handle_t;
typedef DWORD process_id_t;
#else
#include <sys/types.h>
typedef pid_t process_handle_t;
typedef pid_t process_id_t;
#endif

// =============================================================================
// PROCESS LIFECYCLE
// =============================================================================

// Create and execute a child process
// Returns: process handle on success, invalid handle on error
BONGOCAT_NODISCARD process_handle_t
process_spawn(const char *program, char *const argv[]);

// Wait for a process to exit (blocking)
// Returns: exit code of process, or -1 on error
int process_wait(process_handle_t handle);

// Wait for any child process to exit (blocking)
// Returns: handle of exited process, or invalid handle on error
BONGOCAT_NODISCARD process_handle_t process_wait_any(int *exit_code);

// Terminate a process
// Returns: true on success, false on error
bool process_terminate(process_handle_t handle);

// Check if a process handle is valid
bool process_is_valid(process_handle_t handle);

// Get invalid/null process handle
process_handle_t process_invalid_handle(void);

// =============================================================================
// PROCESS IDENTIFICATION
// =============================================================================

// Get current process ID
process_id_t process_get_current_pid(void);

// Get process ID from handle
process_id_t process_get_pid(process_handle_t handle);

// =============================================================================
// SINGLETON/MUTEX MANAGEMENT
// =============================================================================

// Create a named mutex/lock to ensure only one instance runs
// Returns: handle to mutex, or invalid handle if already exists
BONGOCAT_NODISCARD process_handle_t
process_create_singleton_lock(const char *name);

// Check if singleton lock is already held by another process
bool process_singleton_exists(const char *name);

// Release singleton lock
void process_release_singleton_lock(process_handle_t handle);

// Get PID of process holding singleton lock
// Returns: PID or -1 if not found/not held
process_id_t process_get_singleton_holder(const char *name);

#endif  // PLATFORM_PROCESS_H
