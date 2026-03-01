# Building BongoCat

This document describes how to build BongoCat for Linux and Windows.

## Table of Contents

- [Building on Linux (Native)](#building-on-linux-native)
- [Cross-Compiling for Windows (on Linux)](#cross-compiling-for-windows-on-linux)
- [Build System Overview](#build-system-overview)
- [Troubleshooting](#troubleshooting)

---

## Building on Linux (Native)

### Prerequisites

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake wayland wayland-protocols
```

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake libwayland-dev wayland-protocols
```

### Quick Build

```bash
# Release build (optimized)
./scripts/build-linux.sh

# Debug build (with symbols and sanitizers)
./scripts/build-linux.sh --debug

# Clean build
./scripts/build-linux.sh --clean
```

### Manual Build

```bash
mkdir -p build/linux
cd build/linux
cmake ../..
make -j$(nproc)
```

### Installing

```bash
sudo cmake --install build/linux
# Or specify custom prefix:
# cmake --install build/linux --prefix /usr/local
```

The following will be installed:
- `/usr/local/bin/bongocat` - Main executable
- `/usr/local/bin/bongocat-find-devices` - Device detection script
- `/usr/local/share/bongocat/bongocat.conf.example` - Example config
- `/usr/local/share/man/man1/bongocat.1` - Man page

---

## Cross-Compiling for Windows (on Linux)

### Prerequisites

You need the MinGW-w64 toolchain and pthreads-win32 library.

**Arch Linux:**
```bash
sudo pacman -S mingw-w64-gcc
yay -S mingw-w64-pthreads-git
```

**Ubuntu/Debian:**
```bash
sudo apt install mingw-w64
```

For pthreads-win32 on Ubuntu/Debian, you may need to build from source:
```bash
# Download from https://sourceforge.net/projects/pthreads4w/
wget https://sourceforge.net/projects/pthreads4w/files/pthreads-w32-2-9-1-release.tar.gz
tar xzf pthreads-w32-2-9-1-release.tar.gz
cd pthreads-w32-2-9-1-release
make clean GC-static CROSS=x86_64-w64-mingw32-
# Copy libpthreadGC2.a to /usr/x86_64-w64-mingw32/lib/
sudo cp libpthreadGC2.a /usr/x86_64-w64-mingw32/lib/libpthread.a
sudo cp pthread.h /usr/x86_64-w64-mingw32/include/
```

### Quick Build

```bash
# Release build (optimized)
./scripts/build-windows.sh

# Debug build
./scripts/build-windows.sh --debug

# Clean build
./scripts/build-windows.sh --clean
```

### Manual Build

```bash
mkdir -p build/windows
cd build/windows
cmake -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw64.cmake ../..
make -j$(nproc)
```

### Testing the Windows Build

**Option 1: Using Wine (on Linux)**
```bash
wine build/windows/bongocat.exe --version
```

**Option 2: On a Windows Machine**
1. Copy `build/windows/bongocat.exe` to Windows
2. Copy `bongocat.conf.example` to the same directory
3. Install any required DLLs (if not statically linked):
   - `pthreadGC2.dll` (if using dynamic linking)
4. Run `bongocat.exe`

---

## Build System Overview

### CMake Configuration

The build system uses CMake for cross-platform builds:

- **`CMakeLists.txt`** - Main build configuration
- **`cmake/toolchain-mingw64.cmake`** - Windows cross-compilation toolchain

### Platform Detection

CMake automatically detects the target platform:

- **Linux**: Uses Wayland protocols, native pthreads
- **Windows**: Uses Win32 APIs, pthreads-win32

Platform-specific code is enabled via `#ifdef _WIN32` / `#ifdef __linux__`.

### Build Directories

```
build/
├── linux/          # Native Linux builds
│   └── bongocat
└── windows/        # Cross-compiled Windows builds
    └── bongocat.exe
```

### Build Types

- **Release** (default): Optimized with `-O3`, LTO enabled
- **Debug**: With symbols (`-g3`), AddressSanitizer (Linux only)

### Source Organization

```
src/
├── core/           # Main application logic
├── config/         # Configuration management
├── graphics/       # Animation and rendering
├── network/        # Network communication
├── utils/          # Utility functions
└── platform/       # Platform-specific implementations
    ├── wayland.c           # Linux: Wayland display
    ├── input.c             # Linux: evdev input
    ├── windows_display.c   # Windows: Win32 display (TODO)
    ├── windows_input.c     # Windows: keyboard hooks (TODO)
    └── windows_process.c   # Windows: process management (TODO)
```

---

## Troubleshooting

### Linux Build Issues

**Problem: "wayland-client not found"**
```bash
# Install Wayland development libraries
sudo pacman -S wayland wayland-protocols  # Arch
sudo apt install libwayland-dev           # Ubuntu
```

**Problem: "wayland-scanner not found"**
```bash
sudo pacman -S wayland-protocols          # Arch
sudo apt install wayland-protocols        # Ubuntu
```

**Problem: Protocol generation fails**
```bash
# Check if protocol files exist
ls /usr/share/wayland-protocols/stable/xdg-shell/
# If missing, reinstall wayland-protocols
```

### Windows Cross-Compilation Issues

**Problem: "x86_64-w64-mingw32-gcc not found"**
```bash
# Install MinGW toolchain
sudo pacman -S mingw-w64-gcc              # Arch
sudo apt install mingw-w64                # Ubuntu
```

**Problem: "pthread.h not found" during Windows build**

The pthreads-win32 library is not installed. See [Prerequisites](#prerequisites-1) above.

**Problem: "libpthread.a not found"**
```bash
# Check if library is installed
ls /usr/x86_64-w64-mingw32/lib/libpthread*

# If missing, you may need to create a symlink
sudo ln -s /usr/x86_64-w64-mingw32/lib/libpthreadGC2.a \
           /usr/x86_64-w64-mingw32/lib/libpthread.a
```

**Problem: Windows binary crashes on startup**

Try running with Wine and checking the error:
```bash
wine build/windows/bongocat.exe --version
```

If you see DLL errors, you may need to:
1. Ensure static linking (`-static-libgcc` is set in toolchain file)
2. Copy required DLLs to the same directory as the .exe

### General Issues

**Problem: "Build directory is not clean"**
```bash
# Clean and rebuild
./scripts/build-linux.sh --clean
# Or manually:
rm -rf build/linux
```

**Problem: CMake cache issues**
```bash
# Remove CMake cache
rm -rf build/*/CMakeCache.txt build/*/CMakeFiles
```

**Problem: Compilation is slow**
```bash
# Use more parallel jobs
cmake --build build/linux -- -j8
# Or in build scripts, it uses all cores by default: -j$(nproc)
```

---

## Advanced Usage

### Custom Build Options

```bash
# Specify custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/bongocat build/linux
cmake --install build/linux

# Enable verbose build output
cmake --build build/linux -- VERBOSE=1

# Or use the script:
./scripts/build-linux.sh --verbose
```

### Building with Different Compilers

```bash
# Use Clang instead of GCC
CC=clang cmake -B build/linux
cmake --build build/linux
```

### Cross-Compiling with Custom Toolchain Path

Edit `cmake/toolchain-mingw64.cmake` and modify:
```cmake
set(CMAKE_FIND_ROOT_PATH /path/to/your/mingw)
```

---

## Development Workflow

### Recommended Setup

```bash
# 1. Initial setup
git clone https://github.com/yourusername/wayland-bongocat.git
cd wayland-bongocat

# 2. Create development build with debug symbols
./scripts/build-linux.sh --debug

# 3. Make changes to source code

# 4. Rebuild (incremental)
cmake --build build/linux

# 5. Test
./build/linux/bongocat --help
```

### IDE Integration

CMake generates `compile_commands.json` for IDE integration:
```bash
# Link to project root for LSP/clangd
ln -s build/linux/compile_commands.json .
```

### Code Formatting

```bash
# Format all code (if clang-format is configured)
make format -C build/linux
```

---

## Contributing

When adding new platform-specific code:

1. Add source files to appropriate `PLATFORM_SOURCES` in `CMakeLists.txt`
2. Use `#ifdef _WIN32` / `#ifdef __linux__` for platform-specific code
3. Update build scripts if new dependencies are added
4. Test both Linux and Windows builds (cross-compile)

For more details, see [CONTRIBUTING.md](CONTRIBUTING.md).
