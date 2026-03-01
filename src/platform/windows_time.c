#ifdef _WIN32

#include "platform/platform_time.h"
#include <windows.h>

// Windows FILETIME is 100-nanosecond intervals since Jan 1, 1601
// Unix epoch is Jan 1, 1970
#define FILETIME_TO_UNIX_EPOCH 116444736000000000ULL

int64_t platform_time_get_us(void) {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  
  // Convert FILETIME to 64-bit value
  uint64_t time_100ns = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  
  // Convert to microseconds since Unix epoch
  uint64_t time_us = (time_100ns - FILETIME_TO_UNIX_EPOCH) / 10ULL;
  
  return (int64_t)time_us;
}

int64_t platform_time_get_ms(void) {
  return platform_time_get_us() / 1000LL;
}

// For monotonic time, use QueryPerformanceCounter
static int64_t get_qpc_frequency(void) {
  static int64_t frequency = 0;
  if (frequency == 0) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    frequency = freq.QuadPart;
  }
  return frequency;
}

int64_t platform_time_get_monotonic_us(void) {
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  
  int64_t frequency = get_qpc_frequency();
  
  // Convert to microseconds
  return (counter.QuadPart * 1000000LL) / frequency;
}

int64_t platform_time_get_monotonic_ms(void) {
  return platform_time_get_monotonic_us() / 1000LL;
}

#endif  // _WIN32
