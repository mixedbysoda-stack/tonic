#!/bin/bash
# Tonic AAX Plugin Signing Script — produces a PACE-signed AAX for distribution.

if [ -z "$1" ]; then
    echo "Usage: ./sign_aax.sh YOUR_PACE_PASSWORD"
    exit 1
fi

PASSWORD="$1"
AAX_IN="/Users/soda/Desktop/Tonic/build-release/Tonic_artefacts/Release/AAX/Tonic.aaxplugin"
AAX_OUT="/Users/soda/Desktop/Tonic/build-release/Tonic_artefacts/Release/AAX/Tonic_signed.aaxplugin"

echo "Signing Tonic AAX..."

/Applications/PACEAntiPiracy/Eden/Fusion/Versions/5/bin/wraptool sign \
    --account sodanswishers \
    --wcguid 4F87AA20-437D-11F1-B00E-005056928F3B \
    --password "$PASSWORD" \
    --signid 45C26CF1655F48EBC8A21802BDA053514719E1F0 \
    --in "$AAX_IN" \
    --out "$AAX_OUT" \
    --verbose

if [ $? -eq 0 ]; then
    echo "Signing successful!"
    echo "Signed plugin: $AAX_OUT"
else
    echo "Signing failed. Check the error above."
fi
