# Tonic v2.0 — WebView GUI Spike

This folder is **isolated**. v1.0.0 (live, shipped, in customers' hands) is untouched.

## Goal

Prove out the JUCE 8 + WebView2 / WKWebView pipeline so future Carbonated Audio
plugin GUIs can be built as React apps (designer-grade animations, particle
storms, fluid mockups straight from Magic Patterns) instead of hand-painted
in C++.

## Layout

```
v2-webview/
├── README.md                  ← you are here
├── webview-ui/                ← React + Vite app (the actual GUI)
│   ├── package.json
│   ├── vite.config.ts
│   ├── index.html
│   └── src/
│       ├── main.tsx
│       └── App.tsx
└── plugin-src/                ← JUCE C++ shim that hosts the webview
    ├── WebViewEditor.h
    └── WebViewEditor.cpp
```

## How v1 vs v2 builds are gated

The parent `CMakeLists.txt` got one new option:

```bash
cmake -B build-webview -DBUILD_WEBVIEW=ON
```

When `BUILD_WEBVIEW=ON`:
- `JUCE_WEB_BROWSER=1` is defined
- `WebViewEditor.cpp` is compiled instead of `PluginEditor.cpp`
- The Vite build output (`webview-ui/dist/`) is bundled as JUCE BinaryData
- Plugin links against `juce::juce_gui_extra` which provides `WebBrowserComponent`
- DSP, processor, licensing, activation, PACE flow — **all unchanged**

When `BUILD_WEBVIEW=OFF` (default):
- Vanilla v1.0.0 build, shipped product, zero impact.

## Spike acceptance criteria

1. Plugin loads in Logic / Pro Tools with the React GUI rendered
2. Threshold slider in HTML controls the audio THRESHOLD param
3. C++ pushes input level to JS at 30Hz, JS animates a meter
4. Build works on macOS arm64 + x86_64
5. PACE wrap still succeeds (signing the AAX is unchanged by webview presence)

## Once green-lit

- Port full Magic Patterns design → wire all 8 params via `WebSliderRelay`
- Add particle / glow animations (Three.js or pure CSS / Framer Motion)
- Bump version → 1.1.0 (free update for v1 customers, same activation key)
- Roll the same pattern into Carbonator / De-Sipper / Pour / On Tap as v2 updates
