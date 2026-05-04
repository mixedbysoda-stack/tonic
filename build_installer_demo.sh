#!/bin/bash
set -euo pipefail

# Demo-build installer — pulls from build-release-demo, ships with -Demo- in
# filename so the user can tell which one they grabbed. Same install paths
# as the paid PKG so a later paid install cleanly replaces the demo.

VERSION="${1:-2.0.0}"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="$PROJECT_DIR/dist/tonic-v${VERSION}-demo"
INSTALLER_DIR="$PROJECT_DIR/installer"
PAYLOAD="$INSTALLER_DIR/payload-demo"
PKG_UNSIGNED="$INSTALLER_DIR/Tonic-v${VERSION}-Demo-unsigned.pkg"
PKG_SIGNED="$INSTALLER_DIR/Tonic-v${VERSION}-Demo-Installer.pkg"

SIGN_ID_INSTALLER="Developer ID Installer: Miguel Silverio (K7FRP77ZZK)"
NOTARY_PROFILE="carbonator-notary"

echo "=== Tonic v${VERSION} (DEMO) — macOS Installer ==="
[ -d "$DIST_DIR" ] || { echo "ERROR: $DIST_DIR not found — run distribute_demo first."; exit 1; }

echo "[1/5] Staging payload..."
rm -rf "$PAYLOAD"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/Components"
mkdir -p "$PAYLOAD/Library/Application Support/Avid/Audio/Plug-Ins"
cp -R "$DIST_DIR/VST3/Tonic.vst3"      "$PAYLOAD/Library/Audio/Plug-Ins/VST3/"
cp -R "$DIST_DIR/AU/Tonic.component"   "$PAYLOAD/Library/Audio/Plug-Ins/Components/"
cp -R "$DIST_DIR/AAX/Tonic.aaxplugin"  "$PAYLOAD/Library/Application Support/Avid/Audio/Plug-Ins/"

echo "[2/5] Building unsigned package..."
pkgbuild \
    --root "$PAYLOAD" \
    --identifier "com.carbonatedaudio.tonic.demo.pkg" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKG_UNSIGNED" > /dev/null 2>&1

echo "[3/5] Signing package..."
rm -f "$PKG_SIGNED"
productsign --sign "$SIGN_ID_INSTALLER" "$PKG_UNSIGNED" "$PKG_SIGNED"
rm -f "$PKG_UNSIGNED"

echo "[4/5] Notarizing..."
xcrun notarytool submit "$PKG_SIGNED" --keychain-profile "$NOTARY_PROFILE" --wait 2>&1 | tail -4

echo "[5/5] Stapling..."
xcrun stapler staple "$PKG_SIGNED"

echo
echo "=== Verification ==="
pkgutil --check-signature "$PKG_SIGNED" 2>&1 | head -5
spctl --assess --type install "$PKG_SIGNED" && echo "Gatekeeper: PASSED" || echo "Gatekeeper: FAILED"
echo
echo "Installer: $PKG_SIGNED"
echo "Size: $(du -h "$PKG_SIGNED" | cut -f1)"
