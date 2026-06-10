#!/usr/bin/env bash
# Simple installer script for Lime3DS
# This script attempts to build Lime3DS from source with all required dependencies

set -e

echo "=== Lime3DS Build Helper ==="
echo ""
echo "This script will install Lime3DS from source."
echo "Required: ~2-3 GB disk space, 30-45 minutes build time"
echo ""

# Check prerequisites
echo "Checking prerequisites..."
for cmd in git cmake g++ pkg-config; do
    if ! command -v $cmd &> /dev/null; then
        echo "❌ Missing: $cmd"
        echo "Install with: sudo apt-get install build-essential cmake git pkg-config"
        exit 1
    fi
done
echo "✅ Build tools present"
echo ""

# Install Qt5 and other dependencies
echo "Installing dependencies..."
sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev \
    qt5-qmake \
    qtmultimedia5-dev \
    libssl-dev \
    libboost-all-dev \
    libopus-dev \
    libfmt-dev

echo "✅ Dependencies installed"
echo ""

# Clone and build
BUILD_DIR="${HOME}/lime3ds-build"
if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists. Remove to rebuild:"
    echo "  rm -rf $BUILD_DIR"
    echo ""
    read -p "Continue with existing directory? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
else
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Clone with all submodules
if [ ! -f "CMakeLists.txt" ]; then
    echo "Cloning Lime3DS repository..."
    git clone --recursive https://github.com/lime3ds/lime3ds.git .
else
    echo "Repository already cloned, updating..."
    git submodule update --init --recursive
fi

echo "✅ Repository ready"
echo ""

# Build
echo "Building Lime3DS (this will take 20-45 minutes)..."
mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_QT=ON \
    -DENABLE_QT_TRANSLATION=OFF

make -j$(nproc)

echo ""
echo "✅ Build complete!"
echo ""
echo "Run Lime3DS with:"
echo "  $BUILD_DIR/build/bin/lime3ds"
echo ""
echo "To load the 3DS Meshtastic app:"
echo "  $BUILD_DIR/build/bin/lime3ds /home/indreaven/meshtasticds/out/meshtastic3ds.3dsx"
