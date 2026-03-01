# Build System Setup Complete! 🎉

This document provides a quick overview of what was just set up and how to use it.

## What Was Created

### 1. CMake Build System (`CMakeLists.txt`)
- **Platform detection**: Automatically detects Linux vs Windows
- **Conditional compilation**: Compiles platform-specific code
- **Protocol generation**: Auto-generates Wayland protocols on Linux
- **pthreads-win32 integration**: Links pthread library on Windows
- **Build types**: Debug (with sanitizers) and Release (optimized)

### 2. Cross-Compilation Toolchain (`cmake/toolchain-mingw64.cmake`)
- Configures MinGW-w64 for Windows builds from Linux
- Sets up proper search paths and compiler flags
- Enables static linking for portable Windows executables

### 3. Build Scripts
- **`scripts/build-linux.sh`**: Native Linux build script
- **`scripts/build-windows.sh`**: Windows cross-compilation script
- Both support `--debug`, `--clean`, `--verbose` flags

### 4. Platform Abstraction Headers
- **`include/platform/platform_threads.h`**: Threading abstraction (pthreads)
- **`include/platform/platform_process.h`**: Process management abstraction
- **`include/platform/platform_input.h`**: Input handling abstraction

### 5. Documentation
- **`BUILDING.md`**: Comprehensive build instructions
- **`DEPENDENCIES.md`**: Dependency installation guide
- **`BUILD_SYSTEM_README.md`**: This file!

---

## Quick Start

### Install Dependencies (Pick Your OS)

**Arch Linux:**
```bash
# For native Linux builds
sudo pacman -S base-devel cmake wayland wayland-protocols

# For Windows cross-compilation (optional)
sudo pacman -S mingw-w64-gcc
yay -S mingw-w64-pthreads-git
```

**Ubuntu/Debian:**
```bash
# For native Linux builds
sudo apt install build-essential cmake libwayland-dev wayland-protocols pkg-config

# For Windows cross-compilation (optional)
sudo apt install mingw-w64
# pthreads-win32 requires manual installation - see DEPENDENCIES.md
```

### Build for Linux (Native)

```bash
./scripts/build-linux.sh
# Output: build/linux/bongocat
```

### Build for Windows (Cross-Compile)

```bash
./scripts/build-windows.sh
# Output: build/windows/bongocat.exe
```

---

## Current Status

### ✅ Complete
- [x] CMake build system with platform detection
- [x] pthreads-win32 integration
- [x] Cross-compilation toolchain for MinGW
- [x] Build scripts (Linux and Windows)
- [x] Platform abstraction headers
- [x] Documentation

### 🚧 Pending Implementation
These need to be implemented next:

- [ ] `src/platform/windows_display.c` - Windows overlay rendering
- [ ] `src/platform/windows_input.c` - Windows keyboard hooks
- [ ] `src/platform/windows_process.c` - Windows process management
- [ ] Update existing code to use abstraction headers

---

## Testing the Build System

### Test Linux Build (Should Work Now)

```bash
./scripts/build-linux.sh
./build/linux/bongocat --version
```

Expected output:
```
Bongo Cat Overlay v1.4.0
Built with fast optimizations
```

### Test Windows Cross-Compilation (Will Fail - Expected)

```bash
./scripts/build-windows.sh
```

This will **fail** because the Windows platform implementations don't exist yet:
```
error: src/platform/windows_display.c: No such file or directory
```

This is expected! We've set up the build system, but haven't written the Windows implementations yet.

---

## Next Steps

To complete Windows support, you need to implement:

### 1. Windows Display Layer (`src/platform/windows_display.c`)
- Create overlay window (layered window with transparency)
- Implement rendering using GDI/GDI+/Direct2D
- Handle fullscreen detection
- Multi-monitor support

**Suggested approach**: Start with a simple GDI overlay window

### 2. Windows Input Handling (`src/platform/windows_input.c`)
- Install global keyboard hook (SetWindowsHookEx with WH_KEYBOARD_LL)
- Translate VK codes to Linux key codes
- Trigger animation on key press

**Suggested approach**: Basic hook that just logs key presses first

### 3. Windows Process Management (`src/platform/windows_process.c`)
- Implement functions from `platform_process.h`
- CreateProcess for multi-monitor spawning
- Named mutex for singleton lock
- Process termination and waiting

**Suggested approach**: Start with singleton lock, then process spawning

### 4. Update Existing Code
Once platform implementations exist, update:
- `src/core/main.c` - Use `platform_process.h` instead of direct fork()
- `src/platform/input.c` - Rename to `src/platform/linux_input.c` for clarity
- Add `#ifdef _WIN32` / `#else` blocks where needed

---

## Build System Features

### Compiler Flags

**Release build:**
- `-O3` - Aggressive optimization
- `-flto` - Link-time optimization
- `-march=native` - CPU-specific optimizations (Linux only)
- `-DNDEBUG` - Disable assertions

**Debug build:**
- `-g3` - Full debug symbols
- `-O0` - No optimization
- `-fsanitize=address,undefined` - Runtime error detection (Linux only)

### Platform-Specific Linking

**Linux:**
- `-lwayland-client` - Wayland display
- `-lm` - Math library
- `-lpthread` - POSIX threads

**Windows:**
- `-lws2_32` - Winsock (networking)
- `-lgdi32` - Graphics
- `-luser32` - Window management
- `-lkernel32` - Process APIs
- `-lpthread` - pthreads-win32

### Protocol Generation (Linux)

CMake automatically generates Wayland protocol files before building:
- `xdg-shell-protocol.c/h`
- `zwlr-layer-shell-v1-protocol.c/h`
- `wlr-foreign-toplevel-management-v1-protocol.c/h`
- `xdg-output-unstable-v1-protocol.c/h`

This is done via custom commands that run `wayland-scanner`.

---

## Directory Structure

```
wayland-bongocat/
├── CMakeLists.txt              # Main build configuration
├── cmake/
│   └── toolchain-mingw64.cmake # Windows cross-compilation toolchain
├── scripts/
│   ├── build-linux.sh          # Linux build script
│   └── build-windows.sh        # Windows build script
├── include/
│   └── platform/
│       ├── platform_threads.h  # Threading abstraction
│       ├── platform_process.h  # Process abstraction
│       └── platform_input.h    # Input abstraction
├── src/
│   └── platform/
│       ├── wayland.c           # Linux display (existing)
│       ├── input.c             # Linux input (existing)
│       ├── windows_display.c   # Windows display (TODO)
│       ├── windows_input.c     # Windows input (TODO)
│       └── windows_process.c   # Windows process (TODO)
├── build/
│   ├── linux/                  # Native Linux builds
│   │   └── bongocat
│   └── windows/                # Windows cross-compiled builds
│       └── bongocat.exe
├── BUILDING.md                 # Build instructions
├── DEPENDENCIES.md             # Dependency guide
└── BUILD_SYSTEM_README.md      # This file
```

---

## Common Tasks

### Clean Build
```bash
./scripts/build-linux.sh --clean
# or
rm -rf build/linux
```

### Debug Build with Sanitizers
```bash
./scripts/build-linux.sh --debug
./build/linux/bongocat  # Will catch memory errors, undefined behavior
```

### Verbose Build Output
```bash
./scripts/build-linux.sh --verbose
```

### Install System-Wide
```bash
./scripts/build-linux.sh
sudo cmake --install build/linux
```

### Generate compile_commands.json (for IDE)
```bash
# Already generated by CMake
ls build/linux/compile_commands.json
# Link to project root for clangd/LSP
ln -s build/linux/compile_commands.json .
```

---

## Troubleshooting

### Build fails with "wayland-client not found"
```bash
# Install Wayland development packages
sudo pacman -S wayland wayland-protocols  # Arch
sudo apt install libwayland-dev wayland-protocols  # Ubuntu
```

### Windows build fails with "pthread.h not found"
```bash
# Install pthreads-win32
yay -S mingw-w64-pthreads-git  # Arch
# Ubuntu: see DEPENDENCIES.md for manual installation
```

### "MinGW compiler not found"
```bash
sudo pacman -S mingw-w64-gcc  # Arch
sudo apt install mingw-w64    # Ubuntu
```

For more troubleshooting, see **BUILDING.md** and **DEPENDENCIES.md**.

---

## Integration with Existing Makefile

The old Makefile still works! You can use either:

**Option 1: CMake (new, cross-platform)**
```bash
./scripts/build-linux.sh
```

**Option 2: Makefile (old, Linux-only)**
```bash
make
```

Both produce the same result for Linux builds. CMake is recommended going forward because:
- ✅ Cross-platform support
- ✅ Better dependency detection
- ✅ Cleaner build directory structure
- ✅ Built-in protocol generation
- ✅ Multiple build types (debug/release)

---

## Summary

You now have a complete cross-platform build system ready! 

**What works:**
- ✅ Linux native builds (tested with existing code)
- ✅ Windows cross-compilation toolchain (ready to use)
- ✅ Platform abstraction layer (headers defined)
- ✅ Build scripts and documentation

**What's needed next:**
- 🚧 Implement Windows platform code (display, input, process)
- 🚧 Update existing code to use abstractions
- 🚧 Test on actual Windows

The foundation is solid. Now you can start implementing the Windows-specific functionality!

---

**Questions or Issues?**

See:
- **BUILDING.md** - Detailed build instructions
- **DEPENDENCIES.md** - Dependency installation
- **CMakeLists.txt** - Build configuration

Happy coding! 🐱
