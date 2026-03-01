# =============================================================================
# MinGW-w64 Cross-Compilation Toolchain File
# =============================================================================
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake ..
#
# This toolchain enables building Windows executables from Linux using MinGW

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler configuration
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)

# Target environment paths
set(CMAKE_FIND_ROOT_PATH
    /usr/${TOOLCHAIN_PREFIX}
    /usr/lib/gcc/${TOOLCHAIN_PREFIX}
)

# Adjust the default behavior of the FIND_XXX() commands:
# - Search headers and libraries in the target environment
# - Search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Additional MinGW-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# Ensure static linking of MinGW runtime (makes .exe more portable)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc")

# Windows-specific definitions
add_definitions(-DUNICODE -D_UNICODE)

message(STATUS "MinGW-w64 toolchain configured for cross-compilation")
message(STATUS "Target: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
