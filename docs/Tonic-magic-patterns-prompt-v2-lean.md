# Tonic — Magic Patterns Prompt (v2 LEAN)

**Use:** Sent via MCP after the v1 prompt stalled at line 1072. ~50% the size, same interactive effects preserved.

**Editor URL:** https://www.magicpatterns.com/c/6umw4frfsnpusqsdxotsqr
**Preview URL:** https://project-endearing-chocolate-270.magicpatterns.app
**Sent:** 2026-04-28

---

Build a React component `Tonic.tsx` for a vintage opto-tube vocal compressor plugin GUI named **Tonic** by Carbonated Audio. Aesthetic: 1970s lab equipment chassis (brushed champagne brass + ivory cream front panel) sitting in a dark room under blacklight — accents glow fluorescent yellow `#D4FF00` and electric violet `#7B2FFF`. Background `#0A0F1F`. Brass `#C9A876`. Ivory `#F5EFE0`. Tube amber `#FFB347`. Clipping flash `#FF2D7E`.

**Layout (single 900×500 plugin window, top to bottom):**

1. **Top bar:** Wordmark "TONIC" left in retro-futurist sans (yellow with violet glow bloom), small subtitle "opto-tube vocal compressor" in cream. Right side: bypass toggle + preset dropdown.

2. **Main knob row:** Five large brushed-brass rotary knobs in a horizontal line — `THRESHOLD · RATIO · ATTACK · RELEASE · GAIN`. Cream pointer on each. Knob labels in tracked uppercase cream above. Current value below in fluorescent yellow monospace.

3. **Tubes:** Two simplified vertical glass-vacuum-tube illustrations flanking the knob row (one far left, one far right). Soft amber filament glow inside. **Glow brightens with `gain` prop and shifts hotter orange as `drive` prop increases.**

4. **Below knobs (centered):** 3-position rocker switch `FIX / MAN / FIX+MAN` (vintage toggle look) + small meter source selector `INPUT / OUTPUT / GR`.

5. **HERO ELEMENT — central glass tumbler of glowing tonic water (inline SVG):**
   - **Liquid level** rises and falls smoothly with `outputLevel` prop (0–1)
   - **Bubble stream** density and rise speed scale with `gainReduction` prop (0–1) — more GR = denser, faster, more frenzied bubbles
   - **Foam meniscus** appears when `gainReduction > 0.7`, visually overflows when `gainReduction > 0.9`
   - Liquid glows fluorescent yellow `#D4FF00` from within with soft violet rim light

6. **Bottom QoL strip:** smaller compact controls in a row — `MIX | STEREO MODE (Stereo/Dual-Mono/M-S) | SC HPF | DRIVE | OS (1×/2×/4×) | EXT SC`. Brushed brass minis with cream labels.

7. **UV halo:** Soft electric-violet `#7B2FFF` glow surrounds the entire chassis. **Pulses subtly with `inputLevel` prop.** When `clipping === true`, a brief magenta `#FF2D7E` flash overlays the halo (~150ms fade) and the chassis briefly shakes 1–2px.

**Props (TypeScript):**

```ts
type TonicProps = {
  threshold: number; ratio: number; attack: number; release: number; gain: number;
  timeMode: 'FIX'|'MAN'|'FIX+MAN'; meterSource: 'INPUT'|'OUTPUT'|'GR';
  mix: number; stereoMode: 'STEREO'|'DUAL_MONO'|'M_S';
  scHpf: number; drive: number; oversampling: 1|2|4;
  externalSc: boolean; bypass: boolean;
  // Live meter values 0-1
  inputLevel: number; outputLevel: number; gainReduction: number; clipping: boolean;
  onParamChange: (param: string, value: any) => void;
};
```

**Stack:** TypeScript + Tailwind + framer-motion. Inline SVG only (no external assets). Self-contained, default export, include sample default props so it renders standalone in App.tsx.

**Personality:** Idle = slow soft pulse like tonic water under a blacklight in a quiet bar. Lightly compressing = bubbles flow musically, halo gently pulses with rhythm. Heavily compressing = bubbles frenzied, foam visible, halo pulses harder, tube glow shifts orange. Suggestive, not annoying.
