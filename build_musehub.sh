#!/bin/bash
# Tonic — MuseHub Distribution Build
# Produces clean, unwrapped VST3/AU/AAX binaries for MuseHub DRM wrapping.
# Keeps the existing ./build/ and ./build-demo/ directories completely untouched.
#
# Outputs:
#   build-musehub/      — CMake build tree
#   dist-musehub/       — labeled unwrapped binaries ready for Muse Dev Utility
#
# Note: internal CMake product name is "Tonic"; branding is "Tonic".
# Binaries produced are named Tonic.vst3 etc.
#
# Usage:
#   ./build_musehub.sh           # clean build
#   ./build_musehub.sh --fast    # incremental rebuild

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

PRODUCT_NAME="Tonic"
CMAKE_TARGET="Tonic"
BUILD_DIR="build-musehub"
DIST_DIR="dist-musehub"
LABEL="MuseHub"

echo "=================================="
echo "  Tonic MuseHub Build"
echo "  Target: $PRODUCT_NAME ($LABEL distribution)"
echo "=================================="

if [[ "$1" != "--fast" ]]; then
    echo "[1/4] Clean: removing $BUILD_DIR and $DIST_DIR"
    rm -rf "$BUILD_DIR" "$DIST_DIR"
fi

echo "[2/4] CMake configure → $BUILD_DIR"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCOPY_AFTER_BUILD=OFF \
    -DBUILD_DEMO=OFF \
    -DBUILD_MUSEHUB=ON \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -G "Unix Makefiles"

echo "[3/4] Compile (Release)"
cmake --build "$BUILD_DIR" --config Release -j

echo "[4/4] Package unwrapped artifacts → $DIST_DIR"
mkdir -p "$DIST_DIR/VST3" "$DIST_DIR/AU" "$DIST_DIR/AAX" "$DIST_DIR/Standalone"

ARTE="$BUILD_DIR/${CMAKE_TARGET}_artefacts/Release"

if [[ -d "$ARTE/VST3/${PRODUCT_NAME}.vst3" ]]; then
    cp -R "$ARTE/VST3/${PRODUCT_NAME}.vst3" "$DIST_DIR/VST3/"
    echo "  ✓ VST3 → $DIST_DIR/VST3/${PRODUCT_NAME}.vst3"
fi
if [[ -d "$ARTE/AU/${PRODUCT_NAME}.component" ]]; then
    cp -R "$ARTE/AU/${PRODUCT_NAME}.component" "$DIST_DIR/AU/"
    echo "  ✓ AU → $DIST_DIR/AU/${PRODUCT_NAME}.component"
fi
if [[ -d "$ARTE/AAX/${PRODUCT_NAME}.aaxplugin" ]]; then
    cp -R "$ARTE/AAX/${PRODUCT_NAME}.aaxplugin" "$DIST_DIR/AAX/"
    echo "  ✓ AAX → $DIST_DIR/AAX/${PRODUCT_NAME}.aaxplugin (UNWRAPPED — no PACE)"
fi
if [[ -d "$ARTE/Standalone/${PRODUCT_NAME}.app" ]]; then
    cp -R "$ARTE/Standalone/${PRODUCT_NAME}.app" "$DIST_DIR/Standalone/"
    echo "  ✓ Standalone → $DIST_DIR/Standalone/${PRODUCT_NAME}.app"
fi

touch "$DIST_DIR/.MUSEHUB_BUILD"
echo "MuseHub unwrapped build — produced $(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$DIST_DIR/README.txt"
echo "This directory contains UNWRAPPED binaries for MuseHub DRM wrapping." >> "$DIST_DIR/README.txt"
echo "Do NOT install these to /Library/Audio/Plug-Ins — they are not code-signed and not PACE-wrapped." >> "$DIST_DIR/README.txt"
echo "Internal product name is 'Tonic' — branding is 'Tonic'. Set the MuseHub product title accordingly." >> "$DIST_DIR/README.txt"

echo ""
echo "✅ Build complete. Outputs: $SCRIPT_DIR/$DIST_DIR/"
echo "   Ready for Muse Dev Utility wrapping."
