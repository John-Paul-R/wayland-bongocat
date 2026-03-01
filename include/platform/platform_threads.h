#ifndef PLATFORM_THREADS_H
#define PLATFORM_THREADS_H

// =============================================================================
// CROSS-PLATFORM THREADING ABSTRACTION
// =============================================================================
// Provides a uniform pthread-like interface on both Linux and Windows
// On Windows, uses pthreads-win32 library

#ifdef _WIN32
// Windows: Use pthreads-win32
#include <pthread.h>
#else
// Linux: Native pthreads
#include <pthread.h>
#endif

// The threading API is identical on both platforms when using pthreads-win32
// No additional abstraction needed - just include the right header

#endif  // PLATFORM_THREADS_H
