# Tonic — Design Spec

**Date:** 2026-04-28
**Status:** Design locked through DSP + naming + positioning. GUI direction TBD.
**Plugin slug:** `tonic`
**Brand:** Carbonated Audio

---

## One-line pitch

The CL-1B vocal sound — every modern QoL feature, real-DSP modeling — for $20 instead of $200.

## Inspiration (not clone)

Tonic is **inspired by** the Tube-Tech CL-1B opto-tube compressor — long-revered as the gold standard for vocal compression. The plugin is not a 1:1 emulation and does not use the Tube-Tech name in marketing. We draw on the topology (input transformer → opto cell → tube push-pull → output transformer), the iconic 5-knob control set, and the dual time-circuit (FIX / MAN / FIX+MAN) that made the original feel so forgiving on vocals.

## Target user

- Bedroom producers and indie engineers who want a "set it and forget it" vocal compressor that sounds professional out of the box.
- Pros who want a CL-1B vibe in the box without paying UAD/Softube prices.
- Carbonated Audio existing customers — pairs with **De-Sipper** as a complete vocal chain.

## DSP architecture

### Signal flow

```
INPUT (stereo or M/S split)
   │
   ▼
[Input Transformer Model]   — static-screened iron, gentle HF rolloff @ ~25 kHz, mild saturation
   │
   ▼
[Opto Cell — gain reduction] ←── SIDECHAIN DETECTOR
   │                                ├─ Internal or External SC
   │                                ├─ HPF (20 Hz–500 Hz, defeat)
   │                                └─ Dual time-control:
   │                                     ├─ FIX  (program-dependent envelope)
   │                                     ├─ MAN  (user Attack / Release)
   │                                     └─ FIX+MAN (both blended)
   ▼
[Tube Push-Pull Amplifier]  — variable gain 0–30 dB, even-harmonic dominance at low drive,
   │                          shifts toward odd harmonics as DRIVE pushes it
   ▼
[Output Transformer Model]  — second iron stage, weight + low-mid character
   │
   ▼
[DRIVE Stage]               — optional extra tube push (0 = stock CL-1B behavior)
   │
   ▼
[Selective 4× Oversampling] — gain-cell only; bit-transparent when no GR
   │
   ▼
[M/S Matrix]                — applied if Stereo Mode = M-S
   │
   ▼
[Dry/Wet Mix]               — parallel comp
   │
   ▼
[Invisible TP Brickwall]    — -0.1 dBFS, 4× OS, only catches stray peaks; defeat-able in Pro
   │
   ▼
OUTPUT
```

### Hidden intelligence (running automatically)

1. **FIX mode envelope** — true program-dependent: faster release on dense passages, slower on sparse ones (LA-2A "memory effect" applied to opto).
2. **Opto cell thermal model** — release time depends on how long the cell has been illuminated (hardware physics, not just an envelope follower).
3. **Stereo intelligence** — auto-detects mono vs wide content; sets sensible default linking.
4. **Tube harmonic balance shift** — even-dominant at low drive, odd-dominant at high drive (real push-pull behavior).
5. **Sidechain auto-gain** — internal SC gets unity-compensated so threshold doesn't drift when HPF engages.

## Control surface

### Main face — the legendary 5-knob CL-1B layout

| Control | Range | Notes |
|---|---|---|
| **THRESHOLD** | Off → -40 dBu (continuous) | Identical to original |
| **RATIO** | 2:1 → 10:1 (continuous) | Continuous, not stepped |
| **ATTACK** | 0.5 ms → 300 ms (continuous) | Slow attack is what makes vocals breathe |
| **RELEASE** | 50 ms → 10 s (continuous) | Slow program-dependent — the magic |
| **GAIN** | 0 → +30 dB | Tube makeup; contributes harmonics when pushed |

Plus the iconic **FIX / MAN / FIX+MAN** time-mode switch — uses the auto/program-dependent envelope, the manual one, or both blended.

**Plus:** In/Out bypass · GR meter (analog VU style) · Meter selector (Input / Output / GR)

### Modern QoL strip (small, secondary on the face)

A discreet bottom strip — visible but unobtrusive:

- **MIX** (dry/wet — parallel comp)
- **STEREO MODE** (Stereo / Dual-Mono / M-S)
- **SC HPF** (sidechain high-pass, 20 Hz – 500 Hz, defeat-able)
- **DRIVE** (subtle — post-comp saturation stage, adds harmonic character without affecting compression behavior; 0 = stock CL-1B output)
- **OS** (1× / 2× / 4× oversampling)
- **EXT SC** (external sidechain enable)

These are optional. Beginner ignores them entirely; pro reaches for them when needed.

## Pricing & positioning

- **Price:** $20 one-time (matches On Tap / De-Sipper / Pour catalog)
- **Bundle:** Joins the bundle → expands from 4-plugin $60 to 5-plugin (proposed $75, maintains ~25% bundle discount). OWNER20 still applies.
- **Vocal chain pairing:** "Tonic + De-Sipper = complete vocal chain for $40" — direct cross-sell on both product pages.
- **Marketing thesis:** "The CL-1B sound for the price of dinner."
- **KVR angle:** Lead the press release with "indie dev does what UAD does for ~1/10 the price" — the anti-vibe-coded-slop framing that's resonating on KVR/BPB right now.

## Format support

- **macOS:** VST3 / AU / AAX (PACE-wrapped)
- **Windows:** VST3 / AAX (GitHub Actions CI — same pipeline as Pour, De-Sipper)
- **PACE Wrap GUID:** TBD (new GUID per plugin)
- **Demo build:** `BUILD_DEMO=ON` flag, 60s audio / 10s mute (same pattern as De-Sipper)

## Open questions / TBD

1. **GUI direction** — visual identity, layout, color palette. Brainstorm pending.
2. **Tube/transformer DSP coefficients** — actual measurement / circuit model fidelity. Reference units or schematics needed for accuracy.
3. **Bundle final price** — proposed $75 for 5-plugin bundle, but BUNDLE25 / BUNDLE30 promo amounts already need adjustment (memory note).
4. **Beta testers** — pull from existing Carbonator buyers (9) or recruit from KVR forum?
5. **Audio demo clips** — dry/wet pairs needed for landing page (gap noted across catalog — wire into all product pages this round).
6. **Comparison page targets** — likely `/tonic-vs-cl-1b`, `/tonic-vs-la-2a`, `/tonic-vs-uad-tube-tech`. Three SEO pages on launch.

## Launch checklist (high-level — full plan via writing-plans skill when ready)

- [ ] GUI design + brand language locked
- [ ] JUCE project scaffold
- [ ] Core DSP implementation (opto cell, tube stage, transformer models)
- [ ] Hidden intelligence layer (FIX envelope, thermal model, stereo intelligence)
- [ ] Demo build flag + license activation
- [ ] macOS sign + notarize + PACE wrap
- [ ] Windows CI build
- [ ] Stripe product + price + payment link (BNPL auto-on)
- [ ] Landing page at `/tonic` on carbonatedaudio.com
- [ ] Audio A/B clips wired into landing page
- [ ] 3× SEO comparison pages
- [ ] Demo email-gated download (GitHub Release)
- [ ] Add to bundle (5-plugin $75)
- [ ] KVR news post (priority #1)
- [ ] Press blast (BPB, MusicRadar, ADSR, Dan Worrall, etc.) via branded HTML
- [ ] Customer announcement BCC blast (filter against suppression list)
- [ ] Action Log entry per step
