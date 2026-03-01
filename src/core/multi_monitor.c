#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "core/multi_monitor.h"

#include "platform/platform_process.h"
#include "utils/error.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int multi_monitor_launch(int argc, char *argv[], const char *config_path,
                         int watch_config, char **output_names,
                         size_t output_count) {
  (void)argc;
  if (!output_names || output_count == 0) {
    bongocat_log_warning("No monitor names configured, using single monitor");
    return -1;
  }

  if (output_count == 1) {
    bongocat_log_info("Only 1 monitor configured, running single instance");
    return -1;
  }

  if (output_count > MULTI_MONITOR_MAX_OUTPUTS) {
    bongocat_log_warning("Configured %zu monitors, truncating to %d",
                         output_count, MULTI_MONITOR_MAX_OUTPUTS);
    output_count = MULTI_MONITOR_MAX_OUTPUTS;
  }

  bongocat_log_info("Multi-monitor mode: launching %zu instances",
                    output_count);

  struct sigaction old_sigchld = {0};
  struct sigaction default_sigchld = {0};
  bool restore_sigchld = false;
  default_sigchld.sa_handler = SIG_DFL;
  sigemptyset(&default_sigchld.sa_mask);
  default_sigchld.sa_flags = 0;
  if (sigaction(SIGCHLD, &default_sigchld, &old_sigchld) == 0) {
    restore_sigchld = true;
  } else {
    bongocat_log_warning("Failed to override SIGCHLD handler: %s",
                         strerror(errno));
  }

  process_handle_t children[MULTI_MONITOR_MAX_OUTPUTS];
  for (size_t i = 0; i < MULTI_MONITOR_MAX_OUTPUTS; i++) {
    children[i] = process_invalid_handle();
  }

  int alive = 0;
  for (size_t i = 0; i < output_count; i++) {
    if (!output_names[i] || output_names[i][0] == '\0') {
      bongocat_log_warning("Skipping empty monitor entry at index %zu", i);
      continue;
    }

    // Build argument list
    char *new_argv[20];
    int idx = 0;

    new_argv[idx++] = argv[0];
    if (config_path) {
      new_argv[idx++] = "-c";
      new_argv[idx++] = (char *)config_path;
    }
    if (watch_config) {
      new_argv[idx++] = "-w";
    }
    new_argv[idx++] = "--monitor";
    new_argv[idx++] = output_names[i];
    new_argv[idx++] = "--multi-monitor-child";
    new_argv[idx] = NULL;

    process_handle_t handle = process_spawn(argv[0], new_argv);
    if (!process_is_valid(handle)) {
      bongocat_log_error("Failed to spawn process for output '%s'", output_names[i]);
      continue;
    }

    children[i] = handle;
    alive++;
    bongocat_log_info("Launched child PID %d for output '%s'", 
                      process_get_pid(handle), output_names[i]);
  }

  if (alive == 0) {
    bongocat_log_error("Failed to launch any multi-monitor child instances");
    if (restore_sigchld) {
      sigaction(SIGCHLD, &old_sigchld, NULL);
    }
    return 1;
  }

  int exit_code = 0;
  while (alive > 0) {
    int child_exit_code = 0;
    process_handle_t result = process_wait_any(&child_exit_code);

    if (!process_is_valid(result)) {
      if (errno == EINTR) {
        // Interrupted by signal, terminate all children
        for (size_t i = 0; i < output_count; i++) {
          if (process_is_valid(children[i])) {
            process_terminate(children[i]);
          }
        }
        continue;
      }
      break;
    }

    // Find which child exited
    for (size_t i = 0; i < output_count; i++) {
      if (children[i] == result) {
        children[i] = process_invalid_handle();
        alive--;

        if (child_exit_code != 0) {
          bongocat_log_warning("Child PID %d exited with code %d", 
                               process_get_pid(result), child_exit_code);
          exit_code = child_exit_code;
        } else {
          bongocat_log_info("Child PID %d exited normally", 
                           process_get_pid(result));
        }
        break;
      }
    }

    // If one child exits, terminate all others
    if (alive > 0 && alive < (int)output_count) {
      bongocat_log_info("A child exited, terminating remaining %d children",
                        alive);
      for (size_t i = 0; i < output_count; i++) {
        if (process_is_valid(children[i])) {
          process_terminate(children[i]);
        }
      }
    }
  }

  if (restore_sigchld) {
    sigaction(SIGCHLD, &old_sigchld, NULL);
  }

  return exit_code;
}
