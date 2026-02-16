#!/bin/bash
# ──────────────────────────────────────────────
# build_pi_deb.sh - Compile ARM binary and package as .deb
# ──────────────────────────────────────────────
# Run this on the Raspberry Pi (native build) or in a
# cross-compilation environment targeting armv7l/armhf.
#
# Prerequisites:
#   - CMake 3.20+
#   - Qt6 dev packages (qt6-base-dev, qt6-declarative-dev, qt6-multimedia-dev)
#   - libvlc-dev
#   - dpkg-deb
# ──────────────────────────────────────────────
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-pi"
VERSION="1.0.2"

# Detect architecture
ARCH=$(dpkg --print-architecture 2>/dev/null || uname -m)
case "$ARCH" in
    armv7l|armhf) DEB_ARCH="armhf" ;;
    aarch64|arm64) DEB_ARCH="arm64" ;;
    *) echo "WARNING: Unexpected architecture '$ARCH', defaulting to armhf"; DEB_ARCH="armhf" ;;
esac

# CRITICAL: Use architecture-specific build directories to avoid cache poisoning
BUILD_DIR="$PROJECT_DIR/build-$DEB_ARCH"
PACKAGE_DIR="$PROJECT_DIR/packaging/nctv-player_${VERSION}_${DEB_ARCH}"

# Explicitly create the DEBIAN control directory
mkdir -p "$PACKAGE_DIR/DEBIAN"

# Create/Copy the control file template
cat > "$PACKAGE_DIR/DEBIAN/control" <<EOF
Package: nctv-player
Version: $VERSION
Section: multimedia
Priority: optional
Architecture: $DEB_ARCH
Depends: libvlc5 (>= 3.0), vlc-plugin-base, libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6widgets6, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-window, qml6-module-qtquick-layouts, handbrake-cli
Maintainer: NCompass TV <dev@ncompasstv.com>
Description: NCTV Digital Signage Player (Native C++/Qt)
 Native C++/Qt6 digital signage player for Raspberry Pi.
 Supports hardware-accelerated 4K HEVC video playback via libVLC,
 multi-zone display layout, automatic HandBrake optimization,
 and crash recovery via systemd watchdog.
Homepage: https://ncompasstv.com
EOF

# Copy and fix the postinst script if it exists in a central location, or create it
if [ -f "$PROJECT_DIR/packaging/postinst" ]; then
    cp "$PROJECT_DIR/packaging/postinst" "$PACKAGE_DIR/DEBIAN/postinst"
elif [ -f "$PROJECT_DIR/packaging/nctv-player_1.0.1_armhf/DEBIAN/postinst" ]; then
    cp "$PROJECT_DIR/packaging/nctv-player_1.0.1_armhf/DEBIAN/postinst" "$PACKAGE_DIR/DEBIAN/postinst"
fi

if [ -f "$PACKAGE_DIR/DEBIAN/postinst" ]; then
    chmod 755 "$PACKAGE_DIR/DEBIAN/postinst"
    sed -i 's/\r$//' "$PACKAGE_DIR/DEBIAN/postinst"
fi

echo "═══════════════════════════════════════════"
echo "  NCTV Player - Raspberry Pi Debian Build"
echo "  Version: $VERSION  Arch: $DEB_ARCH ($ARCH)"
echo "═══════════════════════════════════════════"

# Clean previous build if requested (or if architecture changed)
if [ "$1" = "clean" ] || [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Cleaning building directory to avoid architecture contamination..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "[1/5] Configuring CMake for $DEB_ARCH..."

# Find Qt6 directory on the current system
QT6_DIR_PATH=$(find /usr/lib -name Qt6Config.cmake -print -quit | xargs dirname)

if [ -z "$QT6_DIR_PATH" ]; then
    echo "WARNING: Could not find Qt6Config.cmake automatically."
    CMAKE_ARGS=""
else
    echo "Found Qt6 at: $QT6_DIR_PATH"
    CMAKE_ARGS="-DQt6_DIR=$QT6_DIR_PATH"
fi

# Explicitly set the platform and processor to prevent CMake from guessing the host arch
# Also ignore x86_64 paths which can leak into ARM containers on Docker Desktop
cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PACKAGE_DIR" \
    -DCMAKE_SYSTEM_PROCESSOR="$DEB_ARCH" \
    -DNCTV_PLATFORM_PI=ON \
    -DCMAKE_IGNORE_PATH="/usr/lib/x86_64-linux-gnu;/usr/include/x86_64-linux-gnu" \
    $CMAKE_ARGS

echo ""
echo "[2/5] Building..."
# Use 2 threads - balance between speed and QEMU stability. 
# Re-enabled parallel build since the JSON issue was likely a CMake configuration error.
cmake --build . --config Release -- -j2
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi
mkdir -p "$PACKAGE_DIR/usr/bin"
cp "$BUILD_DIR/nctv-player" "$PACKAGE_DIR/usr/bin/nctv-player"
chmod 755 "$PACKAGE_DIR/usr/bin/nctv-player"

# Copy systemd service
echo "[3.5/5] Installing systemd service..."
mkdir -p "$PACKAGE_DIR/lib/systemd/system"
cp "$PROJECT_DIR/packaging/lib/systemd/system/nctv-player.service" \
   "$PACKAGE_DIR/lib/systemd/system/nctv-player.service"

# Copy default config
mkdir -p "$PACKAGE_DIR/etc/nctv-player"
if [ -f "$PROJECT_DIR/config.ini" ]; then
    cp "$PROJECT_DIR/config.ini" "$PACKAGE_DIR/etc/nctv-player/config.ini.default"
fi

echo ""
echo "[4/5] Setting package permissions..."
chmod 755 "$PACKAGE_DIR/DEBIAN/postinst"
find "$PACKAGE_DIR" -type d -exec chmod 755 {} \;

echo ""
echo "[5/5] Building .deb package..."
DEB_FILE="$PROJECT_DIR/nctv-player_${VERSION}_${DEB_ARCH}.deb"
dpkg-deb --build "$PACKAGE_DIR" "$DEB_FILE"

echo ""
echo "═══════════════════════════════════════════"
echo "  BUILD SUCCESSFUL"
echo "  Package: $DEB_FILE"
echo ""
echo "  Install with: sudo dpkg -i $DEB_FILE"
echo "  Or push to Aptly with: ./publish_to_aptly.sh"
echo "═══════════════════════════════════════════"
