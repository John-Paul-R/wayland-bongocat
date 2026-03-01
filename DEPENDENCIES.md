# Dependencies Guide

This document lists all dependencies needed to build BongoCat on different platforms.

## Quick Install Commands

### Arch Linux (Native + Cross-Compilation)

```bash
# Native Linux build
sudo pacman -S base-devel cmake wayland wayland-protocols

# Windows cross-compilation (add these)
sudo pacman -S mingw-w64-gcc
yay -S mingw-w64-pthreads-git
```

### Ubuntu/Debian (Native + Cross-Compilation)

```bash
# Native Linux build
sudo apt update
sudo apt install build-essential cmake libwayland-dev wayland-protocols pkg-config

# Windows cross-compilation (add these)
sudo apt install mingw-w64

# Note: pthreads-win32 needs manual installation, see below
```

---

## Detailed Dependencies

### Native Linux Build

| Dependency | Purpose | Install Command (Arch) | Install Command (Ubuntu) |
|------------|---------|------------------------|--------------------------|
| **gcc/clang** | C compiler | `base-devel` | `build-essential` |
| **cmake** | Build system | `cmake` | `cmake` |
| **wayland-client** | Display protocol | `wayland` | `libwayland-dev` |
| **wayland-protocols** | Protocol definitions | `wayland-protocols` | `wayland-protocols` |
| **wayland-scanner** | Code generator | `wayland-protocols` | `wayland-protocols` |
| **pthread** | Threading | (built-in glibc) | (built-in glibc) |
| **libm** | Math library | (built-in glibc) | (built-in glibc) |
| **pkg-config** | Dependency detection | `pkgconf` | `pkg-config` |

### Windows Cross-Compilation (on Linux)

| Dependency | Purpose | Install Command (Arch) | Install Command (Ubuntu) |
|------------|---------|------------------------|--------------------------|
| **mingw-w64-gcc** | Windows compiler | `mingw-w64-gcc` | `mingw-w64` |
| **pthreads-win32** | Threading on Windows | `mingw-w64-pthreads-git` (AUR) | Manual (see below) |

---

## Installing pthreads-win32

### Arch Linux (Easiest)

```bash
# Install from AUR
yay -S mingw-w64-pthreads-git
# or
paru -S mingw-w64-pthreads-git
```

### Ubuntu/Debian (Manual Build)

Since pthreads-win32 is not in Ubuntu's repos, you need to build it:

```bash
# 1. Download source
cd /tmp
wget https://sourceforge.net/projects/pthreads4w/files/pthreads-w32-2-9-1-release.tar.gz
tar xzf pthreads-w32-2-9-1-release.tar.gz
cd pthreads-w32-2-9-1-release

# 2. Build for MinGW
make clean GC-static CROSS=x86_64-w64-mingw32-

# 3. Install to MinGW toolchain
sudo cp libpthreadGC2.a /usr/x86_64-w64-mingw32/lib/libpthread.a
sudo cp pthread.h sched.h semaphore.h /usr/x86_64-w64-mingw32/include/

# 4. Verify installation
ls /usr/x86_64-w64-mingw32/lib/libpthread.a
ls /usr/x86_64-w64-mingw32/include/pthread.h
```

### Alternative: Pre-built Binary (Ubuntu/Debian)

```bash
# Download pre-built DLL and headers from:
# https://sourceforge.net/projects/pthreads4w/files/

# Extract and copy to MinGW directory:
sudo cp prebuilt/lib/x64/*.a /usr/x86_64-w64-mingw32/lib/
sudo cp prebuilt/include/*.h /usr/x86_64-w64-mingw32/include/
```

---

## Runtime Dependencies

### Linux Runtime

When running on Linux, the following are needed at runtime:

- `wayland` (Wayland compositor must be running)
- `/dev/input/event*` access (add user to `input` group)

```bash
# Add user to input group for keyboard monitoring
sudo usermod -a -G input $USER
# Log out and back in for changes to take effect
```

### Windows Runtime

Windows binaries should be statically linked (no DLL dependencies needed if built correctly).

If using dynamic linking, you'll need:
- `pthreadGC2.dll` (from pthreads-win32)
- MinGW runtime DLLs (if not statically linked)

To ensure static linking, the toolchain file sets:
```cmake
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc")
```

---

## Verifying Dependencies

### Check Linux Dependencies

```bash
# Check if Wayland is installed
pkg-config --modversion wayland-client

# Check if wayland-scanner exists
which wayland-scanner

# Check if protocols are installed
ls /usr/share/wayland-protocols/stable/xdg-shell/
```

### Check MinGW Toolchain

```bash
# Check compiler
x86_64-w64-mingw32-gcc --version

# Check if pthreads is available
echo '#include <pthread.h>' | x86_64-w64-mingw32-gcc -E - > /dev/null 2>&1 && echo "OK" || echo "MISSING"

# List MinGW libraries
ls /usr/x86_64-w64-mingw32/lib/ | grep pthread
```

---

## Optional Dependencies

### Development Tools

For development, you may want:

```bash
# Code formatting
sudo pacman -S clang    # or: sudo apt install clang-format

# Static analysis
sudo pacman -S cppcheck # or: sudo apt install cppcheck

# Debugging
sudo pacman -S gdb      # or: sudo apt install gdb

# Memory profiling
sudo pacman -S valgrind # or: sudo apt install valgrind

# Wine (for testing Windows builds on Linux)
sudo pacman -S wine     # or: sudo apt install wine64
```

### IDE Support

For IDE integration (LSP, clangd):

```bash
# Bear (for compile_commands.json generation)
sudo pacman -S bear     # or: sudo apt install bear

# clangd language server
sudo pacman -S clang    # or: sudo apt install clangd
```

---

## Troubleshooting Missing Dependencies

### "Package wayland-client not found"

**Problem**: CMake can't find Wayland libraries

**Solution**:
```bash
# Check if pkg-config can find it
pkg-config --list-all | grep wayland

# If not found, install development packages
sudo pacman -S wayland wayland-protocols  # Arch
sudo apt install libwayland-dev wayland-protocols  # Ubuntu
```

### "pthread.h: No such file or directory" (Windows build)

**Problem**: pthreads-win32 not installed or not in the right location

**Solution**:
```bash
# Check if header exists
ls /usr/x86_64-w64-mingw32/include/pthread.h

# If missing, install pthreads-win32 (see above)
```

### "undefined reference to pthread_create" (Windows build)

**Problem**: pthreads library not linked properly

**Solution**:
```bash
# Check if library exists
ls /usr/x86_64-w64-mingw32/lib/ | grep pthread

# The CMakeLists.txt should automatically find and link it
# If it doesn't, try creating a symlink:
sudo ln -s /usr/x86_64-w64-mingw32/lib/libpthreadGC2.a \
           /usr/x86_64-w64-mingw32/lib/libpthread.a
```

### "wayland-scanner: command not found"

**Problem**: wayland-scanner not in PATH

**Solution**:
```bash
# Install wayland-protocols package
sudo pacman -S wayland-protocols  # Arch
sudo apt install wayland-protocols  # Ubuntu

# Or specify path in CMakeLists.txt:
# set(WAYLAND_SCANNER /usr/bin/wayland-scanner)
```

---

## Minimal Dependency Set

If you only want to build for Linux (native):

```bash
sudo pacman -S base-devel cmake wayland wayland-protocols
```

If you only want to cross-compile for Windows (from Linux):

```bash
sudo pacman -S mingw-w64-gcc cmake
yay -S mingw-w64-pthreads-git
```

---

## Dependency Versions

Tested with:

| Package | Minimum Version | Recommended |
|---------|----------------|-------------|
| CMake | 3.16 | 3.20+ |
| GCC | 9.0 | 11.0+ |
| Wayland | 1.18 | 1.21+ |
| MinGW-w64 | 8.0 | 10.0+ |
| pthreads-win32 | 2.9.1 | 3.0.0+ |

To check versions:
```bash
cmake --version
gcc --version
pkg-config --modversion wayland-client
x86_64-w64-mingw32-gcc --version
```
