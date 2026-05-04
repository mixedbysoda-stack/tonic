#!/bin/bash
set -euo pipefail

# ============================================================================
# Tonic — macOS Installer Builder
# Signs, notarizes, and staples a .pkg installer for VST3 + AU + AAX
# ============================================================================

VERSION="${1:-1.0.0}"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="$PROJECT_DIR/dist/tonic-v${VERSION}"
INSTALLER_DIR="$PROJECT_DIR/installer"
PAYLOAD="$INSTALLER_DIR/payload"
PKG_UNSIGNED="$INSTALLER_DIR/Tonic-v${VERSION}-unsigned.pkg"
PKG_SIGNED="$INSTALLER_DIR/Tonic-v${VERSION}-Installer.pkg"

SIGN_ID_INSTALLER="Developer ID Installer: Miguel Silverio (K7FRP77ZZK)"
NOTARY_PROFILE="carbonator-notary"

echo "=== Tonic v${VERSION} — macOS Installer ==="
echo ""

if [ ! -d "$DIST_DIR" ]; then
    echo "ERROR: Distribution folder not found: $DIST_DIR"
    echo "Run ./distribute.sh ${VERSION} first."
    exit 1
fi

# --- Step 1: Stage payload ---
echo "[1/5] Staging payload..."
rm -rf "$PAYLOAD"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/Components"
mkdir -p "$PAYLOAD/Library/Application Support/Avid/Audio/Plug-Ins"

cp -R "$DIST_DIR/VST3/Tonic.vst3"      "$PAYLOAD/Library/Audio/Plug-Ins/VST3/"
cp -R "$DIST_DIR/AU/Tonic.component"    "$PAYLOAD/Library/Audio/Plug-Ins/Components/"
cp -R "$DIST_DIR/AAX/Tonic.aaxplugin"   "$PAYLOAD/Library/Application Support/Avid/Audio/Plug-Ins/"

echo "  VST3:  Tonic.vst3"
echo "  AU:    Tonic.component"
echo "  AAX:   Tonic.aaxplugin"
echo ""

# --- Step 2: Build unsigned pkg ---
echo "[2/5] Building unsigned package..."
pkgbuild \
    --root "$PAYLOAD" \
    --identifier "com.carbonatedaudio.tonic.pkg" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKG_UNSIGNED" > /dev/null 2>&1

echo "  Built: $(basename "$PKG_UNSIGNED")"
echo ""

# --- Step 3: Sign pkg ---
echo "[3/5] Signing package..."
rm -f "$PKG_SIGNED"
productsign \
    --sign "$SIGN_ID_INSTALLER" \
    "$PKG_UNSIGNED" \
    "$PKG_SIGNED"

rm -f "$PKG_UNSIGNED"
echo "  Signed: $(basename "$PKG_SIGNED")"
echo ""

# --- Step 4: Notarize ---
echo "[4/5] Submitting for Apple notarization..."
xcrun notarytool submit "$PKG_SIGNED" \
    --keychain-profile "$NOTARY_PROFILE" \
    --wait 2>&1 | tail -4
echo ""

# --- Step 5: Staple ---
echo "[5/5] Stapling notarization ticket..."
xcrun stapler staple "$PKG_SIGNED"
echo ""

# --- Verification ---
echo "=== Verification ==="
pkgutil --check-signature "$PKG_SIGNED" 2>&1 | head -5
echo ""

STAPLE_RESULT=$(xcrun stapler validate "$PKG_SIGNED" 2>&1 | grep -o "worked" || echo "FAILED")
echo "Notarization staple: $STAPLE_RESULT"
echo ""

if spctl --assess --type install "$PKG_SIGNED" 2>&1; then
    echo "Gatekeeper: PASSED"
else
    echo "Gatekeeper: FAILED"
fi
echo ""

SIZE=$(du -h "$PKG_SIGNED" | cut -f1)
echo "Installer: $PKG_SIGNED"
echo "Size: $SIZE"
echo ""
echo "=== Done! Tonic v${VERSION} installer ready ==="
