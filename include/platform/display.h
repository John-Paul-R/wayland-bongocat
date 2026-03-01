#ifndef PLATFORM_DISPLAY_H
#define PLATFORM_DISPLAY_H

#include "config/config.h"
#include "utils/error.h"

#include <signal.h>
#include <stdatomic.h>

// =============================================================================
// PLATFORM DISPLAY ABSTRACTION
// =============================================================================
// This header provides a platform-agnostic display API.
// - Linux: Wayland layer shell implementation
// - Windows: Layered windows with GDI+/Direct2D
//
// Both implementations provide the same external API for overlay rendering.

// =============================================================================
// DISPLAY LIFECYCLE FUNCTIONS
// =============================================================================

// Initialize display connection - must be checked
BONGOCAT_NODISCARD bongocat_error_t display_init(config_t *config);

// Run display event loop - must be checked
BONGOCAT_NODISCARD bongocat_error_t display_run(volatile sig_atomic_t *running);

// Cleanup display resources
void display_cleanup(void);

// =============================================================================
// DISPLAY UTILITY FUNCTIONS
// =============================================================================

// Update configuration (hot-reload support)
void display_update_config(config_t *config);

// Get detected screen width
BONGOCAT_NODISCARD int display_get_screen_width(void);

// Get detected output name
BONGOCAT_NODISCARD const char *display_get_output_name(void);

// Register a per-loop callback executed on display thread
void display_set_tick_callback(void (*callback)(void));

// Get current layer name for logging
BONGOCAT_NODISCARD const char *display_get_current_layer_name(void);

// =============================================================================
// INTERNAL RENDERING (called by animation system)
// =============================================================================

// Draw the overlay bar (called by animation thread)
void draw_bar(void);

// =============================================================================
// SHARED STATE (thread-safe)
// =============================================================================

// Thread-safe state flags accessible from any thread
extern atomic_bool configured;
extern atomic_bool fullscreen_detected;

#endif  // PLATFORM_DISPLAY_H
