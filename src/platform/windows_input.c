#ifdef _WIN32

#include "platform/platform_input.h"
#include "platform/platform_threads.h"
#include "graphics/animation.h"
#include "utils/error.h"

#include <windows.h>
#include <stdio.h>
#include <stdatomic.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

atomic_int *any_key_pressed = NULL;
atomic_int *last_key_code = NULL;

static HHOOK keyboard_hook = NULL;
static pthread_t message_thread;
static DWORD message_thread_id = 0;
static _Atomic bool input_running = false;
static int debug_enabled = 0;

// =============================================================================
// VK CODE TO LINUX KEYCODE TRANSLATION
// =============================================================================

int input_vk_to_keycode(int vk_code) {
    // Map Windows Virtual Key codes to Linux event codes
    switch (vk_code) {
        // Number row
        case VK_ESCAPE: return KEYCODE_ESC;
        case '1': return KEYCODE_1;
        case '2': return KEYCODE_2;
        case '3': return KEYCODE_3;
        case '4': return KEYCODE_4;
        case '5': return KEYCODE_5;
        case '6': return KEYCODE_6;
        case '7': return KEYCODE_7;
        case '8': return KEYCODE_8;
        case '9': return KEYCODE_9;
        case '0': return KEYCODE_0;

        // Top row (QWERTY)
        case 'Q': return KEYCODE_Q;
        case 'W': return KEYCODE_W;
        case 'E': return KEYCODE_E;
        case 'R': return KEYCODE_R;
        case 'T': return KEYCODE_T;
        case 'Y': return KEYCODE_Y;
        case 'U': return KEYCODE_U;
        case 'I': return KEYCODE_I;
        case 'O': return KEYCODE_O;
        case 'P': return KEYCODE_P;

        // Home row (ASDF)
        case 'A': return KEYCODE_A;
        case 'S': return KEYCODE_S;
        case 'D': return KEYCODE_D;
        case 'F': return KEYCODE_F;
        case 'G': return KEYCODE_G;
        case 'H': return KEYCODE_H;
        case 'J': return KEYCODE_J;
        case 'K': return KEYCODE_K;
        case 'L': return KEYCODE_L;

        // Bottom row (ZXCV)
        case 'Z': return KEYCODE_Z;
        case 'X': return KEYCODE_X;
        case 'C': return KEYCODE_C;
        case 'V': return KEYCODE_V;
        case 'B': return KEYCODE_B;
        case 'N': return KEYCODE_N;
        case 'M': return KEYCODE_M;

        // Modifiers
        case VK_LSHIFT: return KEYCODE_LEFTSHIFT;
        case VK_RSHIFT: return KEYCODE_RIGHTSHIFT;
        case VK_LCONTROL: return KEYCODE_LEFTCTRL;
        case VK_RCONTROL: return KEYCODE_RIGHTCTRL;
        case VK_LMENU: return KEYCODE_LEFTALT;
        case VK_RMENU: return KEYCODE_RIGHTALT;

        // Special keys
        case VK_TAB: return KEYCODE_TAB;
        case VK_CAPITAL: return KEYCODE_CAPSLOCK;
        case VK_SPACE: return KEYCODE_SPACE;
        case VK_RETURN: return KEYCODE_ENTER;
        case VK_BACK: return KEYCODE_BACKSPACE;

        default:
            return 0;  // Unknown key
    }
}

// =============================================================================
// KEYBOARD HOOK IMPLEMENTATION
// =============================================================================

static LRESULT CALLBACK keyboard_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT *)lParam;
        
        // Only handle key down events
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            int vk_code = (int)kbd->vkCode;
            int keycode = input_vk_to_keycode(vk_code);

            if (debug_enabled) {
                bongocat_log_debug("Key: VK=%d -> keycode=%d", vk_code, keycode);
            }

            if (keycode > 0) {
                // Update shared state
                atomic_store(last_key_code, keycode);
                atomic_store(any_key_pressed, 1);
                
                // Trigger animation
                animation_trigger();
            }
        }
    }

    // Always call next hook
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
}

// =============================================================================
// MESSAGE LOOP THREAD
// =============================================================================

static void *message_loop_thread(void *arg) {
    (void)arg;

    // Store our Windows thread ID for PostThreadMessage
    message_thread_id = GetCurrentThreadId();

    bongocat_log_info("Installing Windows keyboard hook");

    // Install low-level keyboard hook
    keyboard_hook = SetWindowsHookExA(
        WH_KEYBOARD_LL,
        keyboard_hook_proc,
        GetModuleHandle(NULL),
        0
    );

    if (keyboard_hook == NULL) {
        bongocat_log_error("SetWindowsHookEx failed: error %lu", GetLastError());
        return NULL;
    }

    bongocat_log_info("Keyboard hook installed successfully");

    // Run message loop
    MSG msg;
    while (input_running && GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (keyboard_hook != NULL) {
        UnhookWindowsHookEx(keyboard_hook);
        keyboard_hook = NULL;
    }

    bongocat_log_info("Keyboard hook removed");
    return NULL;
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

bongocat_error_t input_start_monitoring(char **device_paths, int num_devices,
                                        char **device_names, int num_names,
                                        int scan_interval, int enable_debug) {
    // Windows ignores device-specific parameters
    (void)device_paths;
    (void)num_devices;
    (void)device_names;
    (void)num_names;
    (void)scan_interval;

    debug_enabled = enable_debug;

    bongocat_log_info("Initializing Windows input system");

    // Allocate shared memory for key state
    any_key_pressed = (atomic_int *)malloc(sizeof(atomic_int));
    if (!any_key_pressed) {
        bongocat_log_error("Failed to allocate memory for key press state");
        return BONGOCAT_ERROR_MEMORY;
    }
    atomic_store(any_key_pressed, 0);

    last_key_code = (atomic_int *)malloc(sizeof(atomic_int));
    if (!last_key_code) {
        bongocat_log_error("Failed to allocate memory for key code");
        free(any_key_pressed);
        any_key_pressed = NULL;
        return BONGOCAT_ERROR_MEMORY;
    }
    atomic_store(last_key_code, 0);

    // Start message loop thread
    input_running = true;
    if (pthread_create(&message_thread, NULL, message_loop_thread, NULL) != 0) {
        bongocat_log_error("Failed to create message thread");
        free(any_key_pressed);
        free(last_key_code);
        any_key_pressed = NULL;
        last_key_code = NULL;
        input_running = false;
        return BONGOCAT_ERROR_THREAD;
    }

    bongocat_log_info("Windows input system initialized");
    return BONGOCAT_SUCCESS;
}

bongocat_error_t input_restart_monitoring(char **device_paths, int num_devices,
                                          char **device_names, int num_names,
                                          int scan_interval, int enable_debug) {
    // For Windows, just update debug flag
    // Hook doesn't need restarting for config changes
    debug_enabled = enable_debug;
    
    (void)device_paths;
    (void)num_devices;
    (void)device_names;
    (void)num_names;
    (void)scan_interval;

    bongocat_log_info("Windows input monitoring restarted (debug=%d)", enable_debug);
    return BONGOCAT_SUCCESS;
}

void input_cleanup(void) {
    bongocat_log_info("Cleaning up Windows input system");

    if (input_running) {
        input_running = false;

        // Post quit message to stop message loop
        if (message_thread_id != 0) {
            PostThreadMessage(message_thread_id, WM_QUIT, 0, 0);
        }

        // Wait for thread to finish
        pthread_join(message_thread, NULL);
        message_thread_id = 0;
    }

    // Free shared memory
    if (any_key_pressed) {
        free(any_key_pressed);
        any_key_pressed = NULL;
    }

    if (last_key_code) {
        free(last_key_code);
        last_key_code = NULL;
    }

    bongocat_log_info("Windows input system cleanup complete");
}

#endif // _WIN32
