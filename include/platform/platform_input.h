#ifndef PLATFORM_INPUT_H
#define PLATFORM_INPUT_H

// =============================================================================
// CROSS-PLATFORM INPUT ABSTRACTION
// =============================================================================
// Provides global keyboard monitoring on Linux (evdev) and Windows (hooks)

#include "core/bongocat.h"
#include "utils/error.h"

#include <stdbool.h>
#include <stdatomic.h>

// =============================================================================
// SHARED INPUT STATE
// =============================================================================

// Shared memory for key press state (thread-safe)
extern atomic_int *any_key_pressed;

// Last pressed key code for hand mapping (0 = none)
// Uses Linux event codes internally (Windows translates VK codes)
extern atomic_int *last_key_code;

// =============================================================================
// PLATFORM-INDEPENDENT KEY CODES
// =============================================================================
// We use Linux event codes as the canonical internal format
// Windows Virtual Key codes are translated to these

// Number row
#define KEYCODE_ESC          1
#define KEYCODE_1            2
#define KEYCODE_2            3
#define KEYCODE_3            4
#define KEYCODE_4            5
#define KEYCODE_5            6
#define KEYCODE_6            7
#define KEYCODE_7            8
#define KEYCODE_8            9
#define KEYCODE_9            10
#define KEYCODE_0            11

// Top row (QWERTY)
#define KEYCODE_Q            16
#define KEYCODE_W            17
#define KEYCODE_E            18
#define KEYCODE_R            19
#define KEYCODE_T            20
#define KEYCODE_Y            21
#define KEYCODE_U            22
#define KEYCODE_I            23
#define KEYCODE_O            24
#define KEYCODE_P            25

// Home row (ASDF)
#define KEYCODE_A            30
#define KEYCODE_S            31
#define KEYCODE_D            32
#define KEYCODE_F            33
#define KEYCODE_G            34
#define KEYCODE_H            35
#define KEYCODE_J            36
#define KEYCODE_K            37
#define KEYCODE_L            38

// Bottom row (ZXCV)
#define KEYCODE_Z            44
#define KEYCODE_X            45
#define KEYCODE_C            46
#define KEYCODE_V            47
#define KEYCODE_B            48
#define KEYCODE_N            49
#define KEYCODE_M            50

// Modifiers
#define KEYCODE_LEFTSHIFT    42
#define KEYCODE_RIGHTSHIFT   54
#define KEYCODE_LEFTCTRL     29
#define KEYCODE_RIGHTCTRL    97
#define KEYCODE_LEFTALT      56
#define KEYCODE_RIGHTALT     100

// Special keys
#define KEYCODE_TAB          15
#define KEYCODE_CAPSLOCK     58
#define KEYCODE_SPACE        57
#define KEYCODE_ENTER        28
#define KEYCODE_BACKSPACE    14

// =============================================================================
// INPUT MONITORING API
// =============================================================================
// 
// Design note: This API accepts all parameters as raw arguments (not a struct)
// to match what main.c already provides from the config system. Linux-specific
// parameters (device_paths, device_names, scan_interval) are ignored on Windows,
// which uses a global keyboard hook and doesn't need device configuration.

/**
 * Initialize and start input monitoring
 * 
 * Linux: Monitors specified device paths and/or device names with hotplug support
 * Windows: Installs global keyboard hook (device_paths/names ignored)
 * 
 * @param device_paths     Array of device paths (e.g., "/dev/input/event4") - Linux only
 * @param num_devices      Number of device paths
 * @param device_names     Array of device name patterns for hotplug - Linux only
 * @param num_names        Number of device name patterns
 * @param scan_interval    Seconds between device scans (0 = scan once) - Linux only
 * @param enable_debug     Log all keypresses to stdout
 * @return Error code (must be checked)
 */
BONGOCAT_NODISCARD bongocat_error_t
input_start_monitoring(char **device_paths, int num_devices, 
                       char **device_names, int num_names,
                       int scan_interval, int enable_debug);

/**
 * Restart input monitoring with new configuration
 * Useful for hot-reloading device configuration
 * 
 * @param device_paths     New array of device paths - Linux only
 * @param num_devices      Number of device paths
 * @param device_names     New array of device name patterns - Linux only
 * @param num_names        Number of device name patterns
 * @param scan_interval    New scan interval - Linux only
 * @param enable_debug     New debug setting
 * @return Error code (must be checked)
 */
BONGOCAT_NODISCARD bongocat_error_t
input_restart_monitoring(char **device_paths, int num_devices,
                         char **device_names, int num_names,
                         int scan_interval, int enable_debug);

/**
 * Stop and cleanup input monitoring
 * Safe to call multiple times
 */
void input_cleanup(void);

// =============================================================================
// PLATFORM-SPECIFIC HELPERS
// =============================================================================

#ifdef _WIN32
/**
 * Windows: Convert Virtual Key code to Linux event code
 * @param vk_code  Windows VK_* code
 * @return Linux KEY_* code
 */
int input_vk_to_keycode(int vk_code);
#endif

#ifdef __linux__
/**
 * Linux: Get child process PID for signal handling
 * Used by crash handler to kill child process
 * @return Child PID or -1 if not running
 */
int input_get_child_pid(void);
#endif

#endif  // PLATFORM_INPUT_H
