# Tonic — Magic Patterns Prompt

**Use:** Paste the prompt below into Magic Patterns to generate the GUI React component.

**Sent to MP:** 2026-04-28 (build at https://www.magicpatterns.com/c/7qf1qnbklbhsr5ydsplwrg)

---

Design a React component for an audio plugin GUI named **Tonic** — a vintage opto-tube vocal compressor for the brand *Carbonated Audio*. Inspired by the Tube-Tech CL-1B hardware compressor crossed with the visual phenomenon of tonic water glowing blue-violet under blacklight (quinine fluorescence).

### Visual concept

The plugin chassis sits on a dark "blacklight room" background. The unit itself looks like vintage 1970s lab/studio equipment — brushed brass knobs on a cream/ivory front panel, but lit dramatically by an unseen UV light source so that *certain elements glow electric yellow-green and violet*.

### Color palette (use exactly)

- Background: `#0A0F1F` (deep midnight blue-black)
- Primary glow (active values, fluorescent text): `#D4FF00` (tonic-yellow / quinine glow)
- UV halo / accent: `#7B2FFF` (electric violet)
- Tube/circuit warmth: `#FFB347` (amber)
- Clipping / danger flash: `#FF2D7E` (hot magenta) — only on transient peak events
- Chassis: brushed champagne brass `#C9A876` with cream ivory `#F5EFE0` plate

### Layout

Single-window plugin, ~900px × 500px landscape. Top to bottom:

1. **Top bar** — Wordmark "TONIC" left (condensed retro-futurist sans, fluorescent yellow with soft violet bloom), tagline "opto-tube vocal compressor" subtitle in cream. Right side: bypass toggle, preset selector dropdown.

2. **Main knob row** — 5 large rotary knobs in a horizontal arc, brushed brass with cream pointers. Left to right: **THRESHOLD · RATIO · ATTACK · RELEASE · GAIN**. Below each knob, current value in fluorescent yellow monospace. Knob labels above in uppercase tracked cream.

3. **Tube illustrations** flanking the knob row — vertical glass vacuum tubes on far left and far right of the panel. Tubes have a soft amber filament glow that visibly **brightens with output gain** and shifts toward warmer orange as DRIVE pushes. Use SVG with animated filter/blur for the glow.

4. **Time-mode selector + meter switch** — centered below the knob row. A 3-position rocker switch for **FIX / MAN / FIX+MAN** (vintage toggle look). To its right, a small meter source selector: **INPUT / OUTPUT / GR**.

5. **Central interactive VU — the hero element.** A glass tumbler of glowing tonic water, rendered in SVG. Three parameters drive its behavior:
   - **Liquid level** rises and falls with `outputLevel` prop (0–1)
   - **Bubble stream** density and speed scales with `gainReduction` prop (0–1) — more GR = more frenzied bubbles rising through the liquid
   - **Foam at the meniscus** appears when `gainReduction > 0.7`, overflows visually when `gainReduction > 0.9`
   - The tonic water itself glows fluorescent yellow `#D4FF00` from within, with a soft violet rim light

6. **QoL strip (bottom)** — single horizontal row of compact controls: **MIX · STEREO MODE (Stereo / Dual-Mono / M-S) · SC HPF · DRIVE · OS (1×/2×/4×) · EXT SC**. Smaller knobs and switches than the main row, brushed brass with cream labels. Uses approximately 1/3 the visual weight of the main controls.

7. **Ambient UV halo** — the entire chassis sits inside a soft violet `#7B2FFF` glow that pulses subtly with the audio signal level. When `clipping === true`, a quick magenta `#FF2D7E` flash overlays the halo (~150ms fade).

### Component API (props)

```tsx
type TonicProps = {
  // Audio parameters (0-1 normalized unless noted)
  threshold: number;       // -40 to 0 dBu
  ratio: number;           // 2 to 10
  attack: number;          // 0.5 to 300 ms
  release: number;         // 50 to 10000 ms
  gain: number;            // 0 to 30 dB
  timeMode: 'FIX' | 'MAN' | 'FIX+MAN';
  meterSource: 'INPUT' | 'OUTPUT' | 'GR';
  mix: number;             // 0-1
  stereoMode: 'STEREO' | 'DUAL_MONO' | 'M_S';
  scHpf: number;           // 20-500 Hz
  drive: number;           // 0-1
  oversampling: 1 | 2 | 4;
  externalSc: boolean;
  bypass: boolean;

  // Live meter values (driven by audio engine, 0-1)
  inputLevel: number;
  outputLevel: number;
  gainReduction: number;
  clipping: boolean;

  // Callbacks
  onParamChange: (param: string, value: any) => void;
};
```

### Animation library

Use **framer-motion** for smooth knob rotations, halo pulses, and tube glow transitions. Use **SVG + CSS `filter: drop-shadow()` + `feGaussianBlur`** for glow effects. The bubble stream in the central VU should be canvas-rendered or SVG with requestAnimationFrame for smooth particle motion.

### Personality cues

- When idle (no signal): everything pulses *slowly and softly*, like a glass of tonic water sitting on a table under a blacklight in a dim bar.
- When lightly compressing (GR < 30%): bubbles flow musically, tube glow stable, halo gently pulses with rhythm. The unit feels "alive but composed."
- When over-compressing (GR > 70%): bubbles get frenzied, tube glow shifts hot orange, foam visible in the glass, halo pulses harder. *Suggestive, not annoying* — like a friend tapping you on the shoulder, not yelling.
- When clipping: brief magenta flash on the halo, slight chassis "shake" (1-2px translate). Quick fade.

### Reference vibe

Tube-Tech CL-1B layout × Teenage Engineering OP-1 craft × Mac OS X Aqua "lickable" depth × molecular gastronomy bar × neon noir.

### Deliverable

A single self-contained React functional component (`Tonic.tsx`) using TypeScript + Tailwind CSS + framer-motion, with all SVG inline. Export as default. Include sample default props so it renders standalone.
