# Tonic — Opto-Tube Vocal Compressor

By [Carbonated Audio](https://carbonatedaudio.com).

Get the full plugin at **[carbonatedaudio.com/tonic](https://carbonatedaudio.com/tonic)** — $20.

## Releases

- **macOS** — Universal `.pkg` installer, signed with Developer ID + Apple notarized.
- **Windows** — VST3 + Standalone `.exe` installer, built via GitHub Actions on every tag.
- Latest installers are on the [Releases page](https://github.com/mixedbysoda-stack/tonic/releases).

After install, paste your license key into the activation overlay and you're ready.

## Build from source

Source is open for transparency, not for redistribution. The license requires a paid activation key from [carbonatedaudio.com/tonic](https://carbonatedaudio.com/tonic) to unlock continuous audio.

### macOS

```bash
git clone --recurse-submodules https://github.com/mixedbysoda-stack/tonic.git
cd tonic
cd v2-webview/webview-ui && npm install && npm run build && cd ../..
cmake -B build -G Xcode -DBUILD_WEBVIEW=ON
cmake --build build --config Release
```

### Windows (the same flow CI runs)

```cmd
git clone --recurse-submodules https://github.com/mixedbysoda-stack/tonic.git
cd tonic
cd v2-webview\webview-ui && npm ci && npm run build && cd ..\..
cmake -B build -DBUILD_WEBVIEW=ON
cmake --build build --config Release
```

## Support

Questions? Email [hello@carbonatedaudio.com](mailto:hello@carbonatedaudio.com) or check the [FAQ](https://carbonatedaudio.com/faq).
