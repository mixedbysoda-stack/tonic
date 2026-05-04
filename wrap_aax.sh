#!/bin/bash
# Tonic AAX Plugin Wrapping Script — quick dev re-sign + install to Pro Tools.
# Mirrors On Tap / Carbonator / De-Sipper pattern. Uses the shared Carbonated
# Audio WCGUID so Pro Tools recognizes the plugin on every customer machine.

if [ -z "$1" ]; then
    echo "Usage: ./wrap_aax.sh YOUR_PACE_PASSWORD"
    exit 1
fi

PASSWORD="$1"
AAX_IN="/Users/soda/Desktop/Tonic/build-release/Tonic_artefacts/Release/AAX/Tonic.aaxplugin"
AAX_OUT="/Users/soda/Desktop/Tonic/build-release/Tonic_artefacts/Release/AAX/Tonic_wrapped.aaxplugin"
PT_PLUGINS="/Library/Application Support/Avid/Audio/Plug-Ins"

echo "Wrapping Tonic AAX..."

/Applications/PACEAntiPiracy/Eden/Fusion/Versions/5/bin/wraptool wrap \
    --verbose \
    --account sodanswishers \
    --password "$PASSWORD" \
    --wcguid 4F87AA20-437D-11F1-B00E-005056928F3B \
    --in "$AAX_IN" \
    --out "$AAX_OUT"

if [ $? -eq 0 ]; then
    echo "Wrapping successful!"
    echo "Installing to Pro Tools..."
    sudo rm -rf "$PT_PLUGINS/Tonic.aaxplugin"
    sudo cp -R "$AAX_OUT" "$PT_PLUGINS/Tonic.aaxplugin"
    echo "Done! Restart Pro Tools to load Tonic."
else
    echo "Wrapping failed."
fi
