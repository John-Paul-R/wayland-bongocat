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
// These are shared between platform-specific implementations

// Shared memory for key press state (thread-safe)
extern atomic_int *any_key_pressed;

// Last pressed key code for hand mapping (0 = none)
extern atomic_int *last_key_code;

// =============================================================================
// KEY CODE DEFINITIONS (PLATFORM-INDEPENDENT)
// =============================================================================
// We use Linux event codes internally as the canonical format
// Windows Virtual Key codes will be translated to these

// Common key codes (from linux/input-event-codes.h)
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
#define KEYCODE_A            30
#define KEYCODE_S            31
#define KEYCODE_D            32
#define KEYCODE_F            33
#define KEYCODE_G            34
#define KEYCODE_H            35
#define KEYCODE_J            36
#define KEYCODE_K            37
#define KEYCODE_L            38
#define KEYCODE_Z            44
#define KEYCODE_X            45
#define KEYCODE_C            46
#define KEYCODE_V            47
#define KEYCODE_B            48
#define KEYCODE_N            49
#define KEYCODE_M            50
#define KEYCODE_LEFTSHIFT    42
#define KEYCODE_RIGHTSHIFT   54
#define KEYCODE_LEFTCTRL     29
#define KEYCODE_RIGHTCTRL    97
#define KEYCODE_LEFTALT      56
#define KEYCODE_RIGHTALT     100
#define KEYCODE_SPACE        57
#define KEYCODE_TAB          15
#define KEYCODE_CAPSLOCK     58
#define KEYCODE_ENTER        28
#define KEYCODE_BACKSPACE    14

// =============================================================================
// INPUT CONFIGURATION
// =============================================================================

typedef struct {
    // Device identification (Linux-specific, ignored on Windows)
    char **device_paths;      // Array of device paths like "/dev/input/event4"
    int num_devices;          // Number of device paths
    
    // Device name matching (for hotplug)
    char **device_names;      // Array of device name patterns
    int num_names;            // Number of name patterns
    
    // Hotplug configuration
    int scan_interval;        // Seconds between device scans (0 = scan once)
    
    // Debug output
    int enable_debug;         // Log all keypresses
} input_config_t;

// =============================================================================
// INPUT MONITORING API
// =============================================================================

// Initialize input monitoring system
// On Linux: Sets up evdev monitoring with hotplug support
// On Windows: Installs global keyboard hook
BONGOCAT_NODISCARD bongocat_error_t
input_init(const input_config_t *config);

// Start input monitoring
// Creates background thread/process to monitor keyboard
BONGOCAT_NODISCARD bongocat_error_t input_start(void);

// Stop input monitoring
void input_stop(void);

// Cleanup input monitoring resources
void input_cleanup(void);

// Restart input monitoring with new configuration
// Useful for hot-reloading device configuration
BONGOCAT_NODISCARD bongocat_error_t
input_restart(const input_config_t *config);

// =============================================================================
// PLATFORM-SPECIFIC HELPERS
// =============================================================================

#ifdef _WIN32
// Windows: Convert Virtual Key code to Linux event code
int input_vk_to_keycode(int vk_code);
#endif

#ifdef __linux__
// Linux: Get child process PID (for signal handling)
int input_get_child_pid(void);
#endif

#endif  // PLATFORM_INPUT_H
