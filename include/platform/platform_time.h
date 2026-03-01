#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#include <stdint.h>

// =============================================================================
// PLATFORM TIME ABSTRACTION
// =============================================================================
// Simple time wrappers for cross-platform time operations.
// Linux: uses gettimeofday/clock_gettime
// Windows: uses GetSystemTimeAsFileTime/QueryPerformanceCounter

// Get current time in microseconds since epoch
int64_t platform_time_get_us(void);

// Get current time in milliseconds since epoch
int64_t platform_time_get_ms(void);

// Get monotonic time in microseconds (for duration measurement)
int64_t platform_time_get_monotonic_us(void);

// Get monotonic time in milliseconds (for duration measurement)
int64_t platform_time_get_monotonic_ms(void);

#endif  // PLATFORM_TIME_H
