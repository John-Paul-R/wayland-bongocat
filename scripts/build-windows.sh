#!/bin/bash
# =============================================================================
# Cross-compile script for Windows (using MinGW on Linux)
# =============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/windows"
TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain-mingw64.cmake"

# Parse arguments
BUILD_TYPE="Release"
CLEAN=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug      Build in debug mode (default: release)"
            echo "  --clean      Clean build directory before building"
            echo "  --verbose    Enable verbose build output"
            echo "  --help       Show this help message"
            echo ""
            echo "Requirements:"
            echo "  - MinGW-w64 toolchain (x86_64-w64-mingw32-gcc)"
            echo "  - pthreads-win32 library"
            echo ""
            echo "Install on Arch Linux:"
            echo "  sudo pacman -S mingw-w64-gcc"
            echo "  yay -S mingw-w64-pthreads-git"
            echo ""
            echo "Install on Ubuntu/Debian:"
            echo "  sudo apt install mingw-w64 libpthread-stubs0-dev"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "========================================"
echo "Cross-compiling BongoCat for Windows"
echo "========================================"
echo "Build type: $BUILD_TYPE"
echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"
echo "Toolchain: $TOOLCHAIN_FILE"
echo ""

# Check for MinGW compiler
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "ERROR: MinGW-w64 compiler not found!"
    echo ""
    echo "Please install MinGW-w64:"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    exit 1
fi

echo "Found MinGW compiler: $(x86_64-w64-mingw32-gcc --version | head -n1)"

# Check for pthreads-win32 (just a warning, build will try anyway)
if ! x86_64-w64-mingw32-gcc -lpthread -xc /dev/null -o /dev/null 2>/dev/null; then
    echo ""
    echo "WARNING: pthreads-win32 library may not be installed"
    echo "Build may fail. To install:"
    echo "  Arch: yay -S mingw-w64-pthreads-git"
    echo "  Or manually from: https://sourceforge.net/projects/pthreads4w/"
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo ""
echo "Configuring CMake for cross-compilation..."
CMAKE_ARGS=(
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

cmake "${CMAKE_ARGS[@]}" "$PROJECT_DIR"

# Build
echo ""
echo "Building..."
MAKE_ARGS=(-j"$(nproc)")

if [ "$VERBOSE" = true ]; then
    MAKE_ARGS+=(VERBOSE=1)
fi

cmake --build . -- "${MAKE_ARGS[@]}"

# Success
echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo "Executable: $BUILD_DIR/bongocat.exe"
echo ""
echo "To test on Windows:"
echo "  1. Copy bongocat.exe to a Windows machine"
echo "  2. Copy bongocat.conf.example to the same directory"
echo "  3. Run: bongocat.exe --help"
echo ""
echo "To test with Wine (if installed):"
echo "  wine $BUILD_DIR/bongocat.exe --version"
