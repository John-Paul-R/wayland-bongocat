#ifdef _WIN32

#include "platform/platform_process.h"
#include "utils/error.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// PROCESS LIFECYCLE
// =============================================================================

process_handle_t process_spawn(const char *program, char *const argv[]) {
    if (!program || !argv) {
        bongocat_log_error("process_spawn: invalid arguments");
        return NULL;
    }

    // Build command line from argv array
    char cmdline[4096] = {0};
    size_t pos = 0;
    
    for (int i = 0; argv[i] != NULL && pos < sizeof(cmdline) - 1; i++) {
        if (i > 0) {
            cmdline[pos++] = ' ';
        }
        
        // Quote arguments with spaces
        bool needs_quotes = strchr(argv[i], ' ') != NULL;
        if (needs_quotes && pos < sizeof(cmdline) - 1) {
            cmdline[pos++] = '"';
        }
        
        size_t len = strlen(argv[i]);
        if (pos + len < sizeof(cmdline)) {
            memcpy(cmdline + pos, argv[i], len);
            pos += len;
        }
        
        if (needs_quotes && pos < sizeof(cmdline) - 1) {
            cmdline[pos++] = '"';
        }
    }
    cmdline[pos] = '\0';

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // Create process
    if (!CreateProcessA(
            NULL,           // Application name (NULL = use command line)
            cmdline,        // Command line
            NULL,           // Process security attributes
            NULL,           // Thread security attributes
            FALSE,          // Don't inherit handles
            0,              // Creation flags
            NULL,           // Environment
            NULL,           // Current directory
            &si,            // Startup info
            &pi)) {         // Process information
        bongocat_log_error("CreateProcess failed: error %lu", GetLastError());
        return NULL;
    }

    // Close thread handle (we don't need it)
    CloseHandle(pi.hThread);

    return pi.hProcess;
}

int process_wait(process_handle_t handle) {
    if (handle == NULL) {
        return -1;
    }

    DWORD result = WaitForSingleObject(handle, INFINITE);
    if (result != WAIT_OBJECT_0) {
        bongocat_log_error("WaitForSingleObject failed: error %lu", GetLastError());
        return -1;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(handle, &exit_code)) {
        bongocat_log_error("GetExitCodeProcess failed: error %lu", GetLastError());
        return -1;
    }

    CloseHandle(handle);
    return (int)exit_code;
}

process_handle_t process_wait_any(int *exit_code) {
    // Windows doesn't have a direct equivalent to waitpid(-1)
    // This would need to track all child handles
    // For now, return error (multi-monitor on Windows will need refactoring)
    bongocat_log_error("process_wait_any not yet implemented on Windows");
    if (exit_code) {
        *exit_code = -1;
    }
    return NULL;
}

bool process_terminate(process_handle_t handle) {
    if (handle == NULL) {
        return false;
    }

    // Try graceful termination first (send WM_CLOSE)
    // For now, just force terminate
    if (TerminateProcess(handle, 1)) {
        CloseHandle(handle);
        return true;
    }

    return false;
}

bool process_is_valid(process_handle_t handle) {
    if (handle == NULL) {
        return false;
    }

    DWORD exit_code;
    if (!GetExitCodeProcess(handle, &exit_code)) {
        return false;
    }

    // STILL_ACTIVE means process is running
    return exit_code == STILL_ACTIVE;
}

process_handle_t process_invalid_handle(void) {
    return NULL;
}

// =============================================================================
// PROCESS IDENTIFICATION
// =============================================================================

process_id_t process_get_current_pid(void) {
    return GetCurrentProcessId();
}

process_id_t process_get_pid(process_handle_t handle) {
    if (handle == NULL) {
        return 0;
    }
    return GetProcessId(handle);
}

// =============================================================================
// SINGLETON/MUTEX MANAGEMENT
// =============================================================================

process_handle_t process_create_singleton_lock(const char *name) {
    // Create named mutex for singleton instance
    char mutex_name[256];
    snprintf(mutex_name, sizeof(mutex_name), "Local\\BongoCat_%s", 
             name ? name : "default");

    HANDLE mutex = CreateMutexA(NULL, FALSE, mutex_name);
    if (mutex == NULL) {
        bongocat_log_error("CreateMutex failed: error %lu", GetLastError());
        return NULL;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running
        CloseHandle(mutex);
        return NULL;
    }

    return mutex;
}

bool process_singleton_exists(const char *name) {
    char mutex_name[256];
    snprintf(mutex_name, sizeof(mutex_name), "Local\\BongoCat_%s", 
             name ? name : "default");

    HANDLE mutex = OpenMutexA(SYNCHRONIZE, FALSE, mutex_name);
    if (mutex != NULL) {
        CloseHandle(mutex);
        return true;
    }

    return false;
}

void process_release_singleton_lock(process_handle_t handle) {
    if (handle != NULL) {
        CloseHandle(handle);
    }
}

process_id_t process_get_singleton_holder(const char *name) {
    // Windows doesn't provide an easy way to get the PID from a mutex
    // Would need additional synchronization or shared memory
    // Return 0 to indicate "unknown"
    (void)name;
    return 0;
}

#endif // _WIN32
