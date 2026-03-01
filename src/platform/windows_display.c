#ifdef _WIN32

// =============================================================================
// WINDOWS DISPLAY LAYER - FULL IMPLEMENTATION
// =============================================================================
// Implements overlay rendering using layered windows with per-pixel alpha
// Features:
// - Transparent overlay window positioned at screen bottom
// - GDI rendering with 32-bit RGBA bitmap
// - Fullscreen detection (basic: checks for maximized foreground window)
// - Hot-reload configuration support

#include "platform/display.h"
#include "platform/platform_threads.h"
#include "config/config.h"
#include "graphics/animation.h"
#include "utils/error.h"

#include <windows.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

atomic_bool configured = false;
atomic_bool fullscreen_detected = false;

static HWND g_hwnd = NULL;
static HDC g_hdc_mem = NULL;
static HBITMAP g_hbitmap = NULL;
static uint8_t *g_pixels = NULL;
static int g_window_width = 0;
static int g_window_height = 0;
static config_t *g_current_config = NULL;
static volatile sig_atomic_t *g_running = NULL;
static void (*g_tick_callback)(void) = NULL;

static const char *WINDOW_CLASS_NAME = "BongoCatOverlay";
static const char *WINDOW_TITLE = "Bongo Cat";

// =============================================================================
// FULLSCREEN DETECTION
// =============================================================================

static void update_fullscreen_detection(void) {
    HWND fg_window = GetForegroundWindow();
    
    if (!fg_window || fg_window == g_hwnd) {
        atomic_store(&fullscreen_detected, false);
        return;
    }
    
    RECT fg_rect;
    if (!GetWindowRect(fg_window, &fg_rect)) {
        atomic_store(&fullscreen_detected, false);
        return;
    }
    
    // Get the monitor that contains the foreground window
    HMONITOR monitor = MonitorFromWindow(fg_window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {0};
    mi.cbSize = sizeof(MONITORINFO);
    
    if (!GetMonitorInfo(monitor, &mi)) {
        atomic_store(&fullscreen_detected, false);
        return;
    }
    
    // Check if window covers the entire monitor
    bool is_fullscreen = (fg_rect.left <= mi.rcMonitor.left &&
                          fg_rect.top <= mi.rcMonitor.top &&
                          fg_rect.right >= mi.rcMonitor.right &&
                          fg_rect.bottom >= mi.rcMonitor.bottom);
    
    atomic_store(&fullscreen_detected, is_fullscreen);
}

// =============================================================================
// RENDERING
// =============================================================================

void draw_bar(void) {
    if (!atomic_load(&configured)) {
        return;
    }
    
    pthread_mutex_lock(&anim_lock);
    
    if (!g_current_config || !g_pixels || !g_hwnd) {
        pthread_mutex_unlock(&anim_lock);
        return;
    }
    
    // Determine visibility and opacity
    bool is_overlay_layer = g_current_config->layer == LAYER_OVERLAY;
    bool is_fullscreen = !is_overlay_layer &&
                         !g_current_config->disable_fullscreen_hide &&
                         atomic_load(&fullscreen_detected);
    int effective_opacity = is_fullscreen ? 0 : g_current_config->overlay_opacity;
    
    // Clear buffer with transparency
    int buffer_size = g_window_width * g_window_height * 4;
    memset(g_pixels, 0, buffer_size);
    
    // Set alpha channel
    if (effective_opacity > 0) {
        for (int i = 3; i < buffer_size; i += 4) {
            g_pixels[i] = effective_opacity;
        }
    }
    
    // Draw cat if visible
    if (!is_fullscreen && anim_index >= 0 && anim_index < NUM_FRAMES) {
        int cat_height = g_current_config->cat_height;
        int cat_width = (cat_height * anim_width[anim_index]) / anim_height[anim_index];
        int cat_y = (g_window_height - cat_height) / 2 + g_current_config->cat_y_offset;
        
        int cat_x = 0;
        switch (g_current_config->cat_align) {
        case ALIGN_CENTER:
            cat_x = (g_window_width - cat_width) / 2 + g_current_config->cat_x_offset;
            break;
        case ALIGN_LEFT:
            cat_x = g_current_config->cat_x_offset;
            break;
        case ALIGN_RIGHT:
            cat_x = g_window_width - cat_width + g_current_config->cat_x_offset;
            break;
        }
        
        blit_image_scaled(g_pixels, g_window_width, g_window_height,
                         anim_imgs[anim_index], anim_width[anim_index], 
                         anim_height[anim_index], cat_x, cat_y, cat_width, cat_height);
    }
    
    pthread_mutex_unlock(&anim_lock);
    
    // Update the layered window
    if (g_hdc_mem && g_hwnd) {
        HDC hdc_screen = GetDC(NULL);
        POINT pt_src = {0, 0};
        POINT pt_dst = {0, 0};
        SIZE size = {g_window_width, g_window_height};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        
        // Get window position
        RECT rect;
        GetWindowRect(g_hwnd, &rect);
        pt_dst.x = rect.left;
        pt_dst.y = rect.top;
        
        UpdateLayeredWindow(g_hwnd, hdc_screen, &pt_dst, &size,
                           g_hdc_mem, &pt_src, 0, &blend, ULW_ALPHA);
        
        ReleaseDC(NULL, hdc_screen);
    }
}

// =============================================================================
// WINDOW MANAGEMENT
// =============================================================================

static void update_window_position(void) {
    if (!g_hwnd || !g_current_config) {
        return;
    }
    
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    // Position at bottom of screen
    int x = 0;
    int y = screen_height - g_window_height;
    
    SetWindowPos(g_hwnd, HWND_TOPMOST, x, y, g_window_width, g_window_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static bongocat_error_t create_dib_section(void) {
    if (g_hdc_mem) {
        DeleteDC(g_hdc_mem);
        g_hdc_mem = NULL;
    }
    
    if (g_hbitmap) {
        DeleteObject(g_hbitmap);
        g_hbitmap = NULL;
    }
    
    HDC hdc_screen = GetDC(NULL);
    g_hdc_mem = CreateCompatibleDC(hdc_screen);
    ReleaseDC(NULL, hdc_screen);
    
    if (!g_hdc_mem) {
        bongocat_log_error("Failed to create memory DC");
        return BONGOCAT_ERROR_DISPLAY;
    }
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_window_width;
    bmi.bmiHeader.biHeight = -g_window_height; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    g_hbitmap = CreateDIBSection(g_hdc_mem, &bmi, DIB_RGB_COLORS,
                                 (void**)&g_pixels, NULL, 0);
    
    if (!g_hbitmap || !g_pixels) {
        bongocat_log_error("Failed to create DIB section");
        return BONGOCAT_ERROR_DISPLAY;
    }
    
    SelectObject(g_hdc_mem, g_hbitmap);
    
    bongocat_log_info("Created DIB section: %dx%d", g_window_width, g_window_height);
    return BONGOCAT_SUCCESS;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        draw_bar();
        EndPaint(hwnd, &ps);
        return 0;
    }
    
    case WM_DISPLAYCHANGE:
        bongocat_log_info("Display configuration changed");
        update_window_position();
        return 0;
        
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

static bongocat_error_t create_overlay_window(void) {
    bongocat_log_debug("create_overlay_window: Starting");
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance) {
        bongocat_log_error("GetModuleHandle returned NULL");
        return BONGOCAT_ERROR_DISPLAY;
    }
    bongocat_log_debug("GetModuleHandle OK: %p", hInstance);
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;  // No background brush for layered window
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    bongocat_log_debug("Registering window class: %s", WINDOW_CLASS_NAME);
    if (!RegisterClassExA(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            bongocat_log_error("Failed to register window class: %lu", err);
            return BONGOCAT_ERROR_DISPLAY;
        }
        bongocat_log_debug("Window class already registered, continuing");
    } else {
        bongocat_log_debug("Window class registered successfully");
    }
    
    // Create layered window
    bongocat_log_debug("Creating window: %dx%d", g_window_width, g_window_height);
    
    g_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_POPUP,
        0, 0, g_window_width, g_window_height,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hwnd) {
        DWORD err = GetLastError();
        bongocat_log_error("CreateWindowExA failed: %lu (0x%08lX)", err, err);
        bongocat_log_error("  Class: %s", WINDOW_CLASS_NAME);
        bongocat_log_error("  Title: %s", WINDOW_TITLE);
        bongocat_log_error("  Size: %dx%d", g_window_width, g_window_height);
        return BONGOCAT_ERROR_DISPLAY;
    }
    
    bongocat_log_debug("CreateWindowExA succeeded: hwnd=%p", g_hwnd);
    
    update_window_position();
    bongocat_log_debug("Window position updated");
    
    ShowWindow(g_hwnd, SW_SHOW);
    bongocat_log_debug("ShowWindow called");
    
    bongocat_log_info("Created overlay window");
    return BONGOCAT_SUCCESS;
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

bongocat_error_t display_init(config_t *config) {
    bongocat_log_debug("display_init: Entry");
    
    if (!config) {
        bongocat_log_error("display_init: config is NULL");
        return BONGOCAT_ERROR_INVALID_PARAM;
    }
    
    g_current_config = config;
    g_window_width = config->screen_width;
    g_window_height = config->bar_height;
    
    bongocat_log_info("Initializing Windows display: %dx%d",
                     g_window_width, g_window_height);
    
    // Create DIB section for rendering
    bongocat_log_debug("display_init: Creating DIB section");
    bongocat_error_t result = create_dib_section();
    if (result != BONGOCAT_SUCCESS) {
        bongocat_log_error("display_init: create_dib_section failed");
        return result;
    }
    bongocat_log_debug("display_init: DIB section created");
    
    // Create overlay window
    bongocat_log_debug("display_init: Creating overlay window");
    result = create_overlay_window();
    if (result != BONGOCAT_SUCCESS) {
        bongocat_log_error("display_init: create_overlay_window failed");
        return result;
    }
    bongocat_log_debug("display_init: Overlay window created");
    
    atomic_store(&configured, true);
    
    bongocat_log_info("Windows display initialized successfully");
    return BONGOCAT_SUCCESS;
}

bongocat_error_t display_run(volatile sig_atomic_t *running) {
    if (!g_hwnd) {
        bongocat_log_error("Window not initialized");
        return BONGOCAT_ERROR_DISPLAY;
    }
    
    g_running = running;
    
    bongocat_log_info("Starting Windows display event loop");
    
    MSG msg;
    DWORD last_fullscreen_check = GetTickCount();
    DWORD last_draw = GetTickCount();
    const DWORD FULLSCREEN_CHECK_INTERVAL = 500; // Check every 500ms
    const DWORD DRAW_INTERVAL = 16; // ~60 FPS
    
    while (*running) {
        // Process Windows messages (non-blocking)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                *running = 0;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!*running) break;
        
        // Periodic fullscreen detection
        DWORD now = GetTickCount();
        if (now - last_fullscreen_check >= FULLSCREEN_CHECK_INTERVAL) {
            update_fullscreen_detection();
            last_fullscreen_check = now;
        }
        
        // Call tick callback
        if (g_tick_callback) {
            g_tick_callback();
        }
        
        // Periodic redraw
        if (now - last_draw >= DRAW_INTERVAL) {
            draw_bar();
            last_draw = now;
        }
        
        Sleep(1); // Yield CPU
    }
    
    bongocat_log_info("Windows display event loop stopped");
    return BONGOCAT_SUCCESS;
}

void display_cleanup(void) {
    bongocat_log_info("Cleaning up Windows display");
    
    atomic_store(&configured, false);
    
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
    
    if (g_hbitmap) {
        DeleteObject(g_hbitmap);
        g_hbitmap = NULL;
    }
    
    if (g_hdc_mem) {
        DeleteDC(g_hdc_mem);
        g_hdc_mem = NULL;
    }
    
    g_pixels = NULL;
    g_current_config = NULL;
    
    UnregisterClassA(WINDOW_CLASS_NAME, GetModuleHandle(NULL));
    
    bongocat_log_info("Windows display cleanup complete");
}

void display_update_config(config_t *config) {
    if (!config) {
        return;
    }
    
    bongocat_log_info("Updating Windows display configuration");
    
    bool size_changed = (config->screen_width != g_window_width ||
                        config->bar_height != g_window_height);
    
    g_current_config = config;
    
    if (size_changed && g_hwnd) {
        g_window_width = config->screen_width;
        g_window_height = config->bar_height;
        
        // Recreate DIB section with new size
        create_dib_section();
        
        // Update window size and position
        update_window_position();
        
        bongocat_log_info("Window resized to %dx%d", g_window_width, g_window_height);
    }
    
    // Force redraw
    if (g_hwnd) {
        InvalidateRect(g_hwnd, NULL, FALSE);
    }
}

int display_get_screen_width(void) {
    return GetSystemMetrics(SM_CXSCREEN);
}

const char *display_get_output_name(void) {
    return "Primary Monitor";
}

void display_set_tick_callback(void (*callback)(void)) {
    g_tick_callback = callback;
}

const char *display_get_current_layer_name(void) {
    if (g_current_config) {
        return g_current_config->layer == LAYER_OVERLAY ? "OVERLAY" : "TOP";
    }
    return "TOP";
}

#endif // _WIN32
