cmake_minimum_required(VERSION 3.16)
project(bongocat VERSION 1.4.0 LANGUAGES C)

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # Allow GNU extensions for compatibility

# =============================================================================
# COMPILER FLAGS
# =============================================================================

# Base flags for all platforms
set(BASE_CFLAGS
    -Wall -Wextra -Wpedantic -Wformat=2 -Wstrict-prototypes
    -Wmissing-prototypes -Wredundant-decls -Wshadow
    -fstack-protector-strong
)

# Platform-specific flags
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND BASE_CFLAGS
        -Wold-style-definition -Wnested-externs
        -Wmissing-include-dirs -Wlogical-op
        -Wjump-misses-init -Wdouble-promotion
    )
endif()

# Build type specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(${BASE_CFLAGS} -g3 -O0 -DDEBUG)
    if(NOT WIN32)
        # AddressSanitizer only on Linux (MinGW doesn't support it well)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
else()
    add_compile_options(${BASE_CFLAGS} -O3 -DNDEBUG -flto)
    add_compile_definitions(_FORTIFY_SOURCE=2)
    if(NOT WIN32)
        add_compile_options(-fomit-frame-pointer -funroll-loops -finline-functions -march=native)
    endif()
    add_link_options(-flto)
endif()

# =============================================================================
# PLATFORM DETECTION AND DEPENDENCIES
# =============================================================================

message(STATUS "Building for: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")

# Platform-specific sources and libraries
if(WIN32)
    message(STATUS "Windows platform detected")
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+

    set(PLATFORM_SOURCES
        src/platform/windows_display.c
        src/platform/windows_input.c
        src/platform/windows_process.c
        src/platform/windows_time.c
    )

    set(PLATFORM_LIBS
        ws2_32      # Winsock (network)
        gdi32       # Graphics
        user32      # Window management
        kernel32    # Process APIs
    )

    # pthreads-win32 handling
    if(CMAKE_CROSSCOMPILING)
        # When cross-compiling, assume pthreads-win32 is in the toolchain
        find_library(PTHREAD_WIN32_LIB NAMES pthread libpthread pthreadGC2)
        if(PTHREAD_WIN32_LIB)
            list(APPEND PLATFORM_LIBS ${PTHREAD_WIN32_LIB})
            message(STATUS "Found pthreads-win32: ${PTHREAD_WIN32_LIB}")
        else()
            message(WARNING "pthreads-win32 not found, you may need to install it")
        endif()
    else()
        # Native Windows build
        find_package(Threads REQUIRED)
        list(APPEND PLATFORM_LIBS Threads::Threads)
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Linux platform detected")

    set(PLATFORM_SOURCES
        src/platform/linux_display.c
        src/platform/linux_input.c
        src/platform/linux_process.c
        src/platform/linux_time.c
    )

    # Find Wayland
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client)

    set(PLATFORM_LIBS
        ${WAYLAND_LIBRARIES}
        m           # Math library
        pthread     # POSIX threads
    )

    include_directories(${WAYLAND_INCLUDE_DIRS})

    # Generate Wayland protocol files
    set(PROTOCOLS_DIR ${CMAKE_SOURCE_DIR}/protocols)
    set(WAYLAND_PROTOCOLS_DIR /usr/share/wayland-protocols)

    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    # Protocol generation commands
    set(PROTOCOL_SOURCES)
    set(PROTOCOL_HEADERS)

    # XDG Shell
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/xdg-shell-protocol.c
        COMMAND ${WAYLAND_SCANNER} private-code
            ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
            ${PROTOCOLS_DIR}/xdg-shell-protocol.c
        DEPENDS ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
    )
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/xdg-shell-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} client-header
            ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
            ${PROTOCOLS_DIR}/xdg-shell-client-protocol.h
        DEPENDS ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
    )

    # Layer Shell
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-protocol.c
        COMMAND ${WAYLAND_SCANNER} private-code
            ${PROTOCOLS_DIR}/wlr-layer-shell-unstable-v1.xml
            ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-protocol.c
        DEPENDS ${PROTOCOLS_DIR}/wlr-layer-shell-unstable-v1.xml
    )
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} client-header
            ${PROTOCOLS_DIR}/wlr-layer-shell-unstable-v1.xml
            ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-client-protocol.h
        DEPENDS ${PROTOCOLS_DIR}/wlr-layer-shell-unstable-v1.xml
    )

    # Foreign Toplevel
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-protocol.c
        COMMAND ${WAYLAND_SCANNER} private-code
            ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml
            ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-protocol.c
        DEPENDS ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml
    )
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} client-header
            ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml
            ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-client-protocol.h
        DEPENDS ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml
    )

    # XDG Output
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/xdg-output-unstable-v1-protocol.c
        COMMAND ${WAYLAND_SCANNER} private-code
            ${PROTOCOLS_DIR}/xdg-output-unstable-v1.xml
            ${PROTOCOLS_DIR}/xdg-output-unstable-v1-protocol.c
        DEPENDS ${PROTOCOLS_DIR}/xdg-output-unstable-v1.xml
    )
    add_custom_command(
        OUTPUT ${PROTOCOLS_DIR}/xdg-output-unstable-v1-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} client-header
            ${PROTOCOLS_DIR}/xdg-output-unstable-v1.xml
            ${PROTOCOLS_DIR}/xdg-output-unstable-v1-client-protocol.h
        DEPENDS ${PROTOCOLS_DIR}/xdg-output-unstable-v1.xml
    )

    list(APPEND PROTOCOL_SOURCES
        ${PROTOCOLS_DIR}/xdg-shell-protocol.c
        ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-protocol.c
        ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-protocol.c
        ${PROTOCOLS_DIR}/xdg-output-unstable-v1-protocol.c
    )

    list(APPEND PROTOCOL_HEADERS
        ${PROTOCOLS_DIR}/xdg-shell-client-protocol.h
        ${PROTOCOLS_DIR}/zwlr-layer-shell-v1-client-protocol.h
        ${PROTOCOLS_DIR}/wlr-foreign-toplevel-management-v1-client-protocol.h
        ${PROTOCOLS_DIR}/xdg-output-unstable-v1-client-protocol.h
    )

else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# =============================================================================
# SOURCE FILES (COMMON)
# =============================================================================

set(COMMON_SOURCES
    src/core/main.c
    src/core/multi_monitor.c
    src/config/config.c
    src/config/config_watcher.c
    src/graphics/animation.c
    src/graphics/embedded_assets.c
    src/network/network.c
    src/utils/error.c
    src/utils/memory.c
)

# =============================================================================
# EXECUTABLE TARGET
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(bongocat
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
        ${PROTOCOL_SOURCES}
    )

    # Ensure protocol headers are generated before compiling
    add_custom_target(wayland-protocols DEPENDS ${PROTOCOL_HEADERS})
    add_dependencies(bongocat wayland-protocols)
else()
    add_executable(bongocat
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
    )
endif()

# Include directories
target_include_directories(bongocat PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/protocols
)

# Link libraries
target_link_libraries(bongocat PRIVATE ${PLATFORM_LIBS})

# =============================================================================
# INSTALL TARGETS
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(TARGETS bongocat DESTINATION bin)
    install(FILES bongocat.conf.example
            DESTINATION share/bongocat)
    install(PROGRAMS scripts/find_input_devices.sh
            DESTINATION bin
            RENAME bongocat-find-devices)
    install(FILES man/bongocat.1
            DESTINATION share/man/man1)
elseif(WIN32)
    install(TARGETS bongocat DESTINATION .)
    install(FILES bongocat.conf.example DESTINATION .)
endif()

# =============================================================================
# HELPFUL TARGETS
# =============================================================================

# Show configuration
add_custom_target(show-config
    COMMAND ${CMAKE_COMMAND} -E echo "Platform: ${CMAKE_SYSTEM_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "Install prefix: ${CMAKE_INSTALL_PREFIX}"
)
