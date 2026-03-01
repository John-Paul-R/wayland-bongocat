#ifdef __linux__

#include "platform/platform_time.h"
#include <sys/time.h>
#include <time.h>

int64_t platform_time_get_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000000LL + (int64_t)tv.tv_usec;
}

int64_t platform_time_get_ms(void) {
  return platform_time_get_us() / 1000LL;
}

int64_t platform_time_get_monotonic_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000000LL + (int64_t)ts.tv_nsec / 1000LL;
}

int64_t platform_time_get_monotonic_ms(void) {
  return platform_time_get_monotonic_us() / 1000LL;
}

#endif  // __linux__
