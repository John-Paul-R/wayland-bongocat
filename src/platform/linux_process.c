#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "platform/platform_process.h"
#include "utils/error.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// =============================================================================
// PROCESS LIFECYCLE
// =============================================================================

process_handle_t process_spawn(const char *program, char *const argv[]) {
    if (!program || !argv) {
        bongocat_log_error("process_spawn: invalid arguments");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        bongocat_log_error("fork failed: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        // Child process
        execvp(program, argv);
        bongocat_log_error("execvp failed for %s: %s", program, strerror(errno));
        _exit(1);
    }

    // Parent process
    return pid;
}

int process_wait(process_handle_t handle) {
    if (handle <= 0) {
        return -1;
    }

    int status;
    if (waitpid(handle, &status, 0) < 0) {
        bongocat_log_error("waitpid failed: %s", strerror(errno));
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);  // Shell convention
    }

    return -1;
}

process_handle_t process_wait_any(int *exit_code) {
    int status;
    pid_t pid = waitpid(-1, &status, 0);
    
    if (pid < 0) {
        if (errno != EINTR && errno != ECHILD) {
            bongocat_log_error("waitpid failed: %s", strerror(errno));
        }
        return -1;
    }

    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = -1;
        }
    }

    return pid;
}

bool process_terminate(process_handle_t handle) {
    if (handle <= 0) {
        return false;
    }

    // Try graceful termination first
    if (kill(handle, SIGTERM) == 0) {
        return true;
    }

    // If SIGTERM fails, try SIGKILL
    if (errno == ESRCH) {
        // Process doesn't exist
        return false;
    }

    return kill(handle, SIGKILL) == 0;
}

bool process_is_valid(process_handle_t handle) {
    if (handle <= 0) {
        return false;
    }

    // Check if process exists
    return kill(handle, 0) == 0 || errno == EPERM;
}

process_handle_t process_invalid_handle(void) {
    return -1;
}

// =============================================================================
// PROCESS IDENTIFICATION
// =============================================================================

process_id_t process_get_current_pid(void) {
    return getpid();
}

process_id_t process_get_pid(process_handle_t handle) {
    // On Linux, handle IS the PID
    return handle;
}

// =============================================================================
// SINGLETON/MUTEX MANAGEMENT
// =============================================================================

#define PID_FILE_PATH "/tmp/bongocat.pid"

process_handle_t process_create_singleton_lock(const char *name) {
    (void)name;  // Name not used on Linux, using fixed PID file path

    int fd = open(PID_FILE_PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        bongocat_log_error("Failed to create PID file: %s", strerror(errno));
        return -1;
    }

    // Try to get exclusive lock
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        int lock_err = errno;
        close(fd);
        
        if (lock_err == EWOULDBLOCK) {
            // Another instance is running
            return -1;
        }
        
        bongocat_log_error("Failed to lock PID file: %s", strerror(lock_err));
        return -1;
    }

    // Write PID to file
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(fd, pid_str, strlen(pid_str)) < 0) {
        bongocat_log_error("Failed to write PID: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // Keep fd open to maintain lock
    return fd;
}

bool process_singleton_exists(const char *name) {
    (void)name;

    int fd = open(PID_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        return false;  // No PID file = no instance running
    }

    // Try to get shared lock
    if (flock(fd, LOCK_SH | LOCK_NB) < 0) {
        close(fd);
        return true;  // Lock held = instance running
    }

    // We got the lock, so no other instance is running
    flock(fd, LOCK_UN);
    close(fd);
    return false;
}

void process_release_singleton_lock(process_handle_t handle) {
    if (handle > 0) {
        close(handle);
        unlink(PID_FILE_PATH);
    }
}

process_id_t process_get_singleton_holder(const char *name) {
    (void)name;

    int fd = open(PID_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    // Try to read PID
    char pid_str[32];
    ssize_t bytes_read = read(fd, pid_str, sizeof(pid_str) - 1);
    close(fd);

    if (bytes_read <= 0) {
        return -1;
    }

    pid_str[bytes_read] = '\0';
    pid_t pid = (pid_t)atoi(pid_str);

    // Verify process is still running
    if (pid > 0 && kill(pid, 0) == 0) {
        return pid;
    }

    return -1;
}
