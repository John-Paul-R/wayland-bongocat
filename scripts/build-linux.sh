#!/bin/bash
# =============================================================================
# Build script for Linux (native build)
# =============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/linux"

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
echo "Building BongoCat for Linux"
echo "========================================"
echo "Build type: $BUILD_TYPE"
echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"
echo ""

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
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
echo "Executable: $BUILD_DIR/bongocat"
echo ""
echo "To install system-wide:"
echo "  sudo cmake --install $BUILD_DIR"
echo ""
echo "To run:"
echo "  $BUILD_DIR/bongocat --help"
