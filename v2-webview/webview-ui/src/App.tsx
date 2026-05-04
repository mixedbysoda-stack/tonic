import { useEffect, useMemo, useState } from "react";
import CyberKnob from "./components/CyberKnob";
import CycleToggle from "./components/CycleToggle";
import BoolToggle from "./components/BoolToggle";
import ParticleField from "./components/ParticleField";
import AuroraBackground from "./components/AuroraBackground";
import MeterBar from "./components/MeterBar";
import StereoMeterBar from "./components/StereoMeterBar";
import PresetBrowser from "./components/PresetBrowser";
import SizePicker from "./components/SizePicker";
import ActivationOverlay from "./components/ActivationOverlay";
import { useMeters } from "./juceBridge";

// ---------------------------------------------------------------------------
// Real-world value formatters. Per the JUCE 8 ParameterAttachment source:
// the slider relay emits DENORMALISED real-world values to JS (e.g. -20 for
// threshold, 100 for mix, 200 for release in ms). The formatters just present
// the value — no scaling needed.
// ---------------------------------------------------------------------------

const fmt = {
  threshold: (v: number) => `${v.toFixed(1)} dB`,
  ratio:     (v: number) => `${v.toFixed(1)}:1`,
  attack:    (v: number) => v < 10 ? `${v.toFixed(2)} ms` : `${v.toFixed(1)} ms`,
  release:   (v: number) => v < 1000 ? `${v.toFixed(0)} ms` : `${(v / 1000).toFixed(2)} s`,
  gain:      (v: number) => `${v >= 0 ? "+" : ""}${v.toFixed(1)} dB`,
  mix:       (v: number) => `${v.toFixed(0)}%`,
  scHpf:     (v: number) => `${v.toFixed(0)} Hz`,
  drive:     (v: number) => v.toFixed(1),
};

// ---------------------------------------------------------------------------

// Track the actual webview viewport size. window.innerWidth is more reliable
// than asking the C++ side because it always reflects whatever the host is
// actually showing the webview at — no IPC round trip, no stale state. We
// scale the chassis based on the SMALLER axis so it always fits.
function useViewportSize() {
  const [size, setSize] = useState({
    w: typeof window !== "undefined" ? window.innerWidth  : 1080,
    h: typeof window !== "undefined" ? window.innerHeight : 640,
  });
  useEffect(() => {
    const onResize = () => setSize({ w: window.innerWidth, h: window.innerHeight });
    window.addEventListener("resize", onResize);
    // Some webviews don't fire a resize on first attach — kick once next tick.
    const t = setTimeout(onResize, 0);
    return () => { window.removeEventListener("resize", onResize); clearTimeout(t); };
  }, []);
  return size;
}

export default function App() {
  const meters = useMeters();
  const { w: vw, h: vh } = useViewportSize();

  // Audio-reactive intensities
  const inputEnergy = meters.input;             // 0..1
  const grHeat = meters.gr01;                   // 0..1, drives compression "redness"
  const ambient = useMemo(
    () => Math.max(meters.input, meters.output),
    [meters.input, meters.output]
  );

  // Scale the 1080×640 chassis to fit the viewport. We use the CSS `zoom`
  // property (rather than transform: scale) because zoom changes the layout
  // box itself — at zoom 0.75, the chassis behaves as if it were 810×480
  // for every layout calculation including overflow. transform: scale only
  // resizes the visual, leaving a 1080×640 layout box behind that fights
  // the viewport. zoom is non-standard but supported by every WebKit and
  // Chromium engine, which covers every JUCE WebBrowserComponent platform.
  const scale = Math.min(vw / 1080, vh / 640);

  return (
    <div className="chassis" style={{ zoom: scale }}>
      {/* Ambient aurora wash behind everything */}
      <AuroraBackground intensity={ambient} />

      {/* Subtle scanlines + vignette overlay (CSS only) */}
      <div className="scanlines" />
      <div className="vignette" />

      {/* Corner crop brackets — pure decoration, anime UI vibe */}
      <div className="crop tl" />
      <div className="crop tr" />
      <div className="crop bl" />
      <div className="crop br" />

      {/* ===================== HEADER ===================== */}
      <header className="header">
        <div className="brand-block">
          <span className="brand-mark" />
          <span className="brand-name">TONIC</span>
          <span className="brand-tag">CARBONATED AUDIO</span>
        </div>

        <div className="header-controls">
          <PresetBrowser />
          <SizePicker />
        </div>
      </header>

      {/* ===================== MAIN BODY ===================== */}
      <div className="body">
        {/* LEFT METER COLUMN — stereo input level */}
        <aside className="meter-col left">
          <StereoMeterBar
            label="IN"
            left={meters.inputL}
            right={meters.inputR}
            peakLeft={meters.inputPeakL}
            peakRight={meters.inputPeakR}
            height={460}
          />
        </aside>

        {/* CENTER STAGE */}
        <main className="stage">
          {/* HERO PANEL — particle storm, central readout, GR meter */}
          <section className="hero">
            <div className="hero-particles">
              <ParticleField intensity={inputEnergy} heat={grHeat} clipping={meters.clipping} />
            </div>

            <div className="hero-center">
              <div className={`clipping-indicator ${meters.clipping ? "on" : ""}`}>
                ! CLIP
              </div>

              <div className="gr-readout">
                <div className="gr-readout-label">GAIN REDUCTION</div>
                <div className="gr-readout-value">
                  -{meters.gainReductionDb.toFixed(1)}
                  <span className="gr-readout-unit"> dB</span>
                </div>
                <div className="gr-bar-h">
                  <div className="gr-bar-h-fill" style={{ width: `${meters.gr01 * 100}%` }} />
                </div>
              </div>
            </div>

            {/* horizontal threshold "rail" — visual reference of audio crossing threshold */}
            <div className="threshold-rail" />
          </section>

          {/* MAIN KNOB ROW — 5 core dynamics controls */}
          <section className="knob-row main">
            <CyberKnob paramId="threshold" label="THRESHOLD" format={fmt.threshold} energy={inputEnergy} />
            <CyberKnob paramId="ratio"     label="RATIO"     format={fmt.ratio}     energy={grHeat * 0.6} />
            <CyberKnob paramId="attack"    label="ATTACK"    format={fmt.attack}    energy={grHeat * 0.4} />
            <CyberKnob paramId="release"   label="RELEASE"   format={fmt.release}   energy={grHeat * 0.4} />
            <CyberKnob paramId="gain"      label="GAIN"      format={fmt.gain}      energy={meters.output * 0.8} hue={50} />
          </section>

          {/* QoL ROW — secondary knobs + cycle toggles */}
          <section className="qol-row">
            <CyberKnob paramId="mix"   label="MIX"     format={fmt.mix}   size={68} />
            <CyberKnob paramId="scHpf" label="SC HPF"  format={fmt.scHpf} size={68} />
            <CyberKnob paramId="drive" label="DRIVE"   format={fmt.drive} size={68} hue={300} />

            <div className="cycle-stack">
              <CycleToggle paramId="timeMode"     label="TIME"   choices={["FIX", "MAN", "FIX+MAN"]} />
              <CycleToggle paramId="meterSource"  label="METER"  choices={["INPUT", "GR", "OUTPUT"]} />
              <CycleToggle paramId="stereoMode"   label="STEREO" choices={["STEREO", "DUAL", "M/S"]} />
              <CycleToggle paramId="oversampling" label="OS"     choices={["1×", "2×", "4×"]} />
            </div>

            <div className="bool-stack">
              <BoolToggle paramId="externalSc" label="EXT SC" glyph="⤵" />
              <BoolToggle paramId="bypass"     label="BYPASS" glyph="◐" variant="warning" />
            </div>
          </section>
        </main>

        {/* RIGHT METER COLUMN — stereo output level + GR */}
        <aside className="meter-col right">
          <MeterBar label="GR" value={meters.gr01} variant="gr" height={460} />
          <StereoMeterBar
            label="OUT"
            left={meters.outputL}
            right={meters.outputR}
            peakLeft={meters.outputPeakL}
            peakRight={meters.outputPeakR}
            height={460}
          />
        </aside>
      </div>

      {/* ===================== FOOTER ===================== */}
      <footer className="footer">
        <span className="footer-mid">OPTO·TUBE COMPRESSOR</span>
      </footer>

      {/* Activation gate — covers everything when not licensed */}
      <ActivationOverlay />
    </div>
  );
}
