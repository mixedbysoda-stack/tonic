import { useCallback, useEffect, useRef, useState } from "react";
import { useSlider } from "../juceBridge";

interface CyberKnobProps {
  paramId: string;            // matches a WebSliderRelay identifier
  label: string;
  size?: number;              // px diameter, default 86
  format?: (realWorld: number) => string;  // formats the real-world value
  energy?: number;            // 0..1 audio-reactive boost on the indicator ring
  hue?: number;               // colour tint for the indicator (default fluo-yellow)
}

const ARC_START = 135;
const ARC_END   = 405;
const ARC_RANGE = ARC_END - ARC_START;

function polar(cx: number, cy: number, r: number, angleDeg: number) {
  const a = (angleDeg * Math.PI) / 180;
  return { x: cx + r * Math.cos(a), y: cy + r * Math.sin(a) };
}
function arcPath(cx: number, cy: number, r: number, startDeg: number, endDeg: number) {
  const start = polar(cx, cy, r, startDeg);
  const end   = polar(cx, cy, r, endDeg);
  const sweep = endDeg - startDeg;
  const largeArc = sweep > 180 ? 1 : 0;
  return `M ${start.x} ${start.y} A ${r} ${r} 0 ${largeArc} 1 ${end.x} ${end.y}`;
}

export default function CyberKnob({
  paramId, label, size = 86, format, energy = 0, hue = 65,
}: CyberKnobProps) {
  const slider = useSlider(paramId);

  const [dragging, setDragging] = useState(false);
  const dragStartY = useRef(0);
  const dragStartNorm = useRef(0);
  const containerRef = useRef<HTMLDivElement>(null);

  const onPointerDown = useCallback((e: React.PointerEvent) => {
    e.preventDefault();
    window.getSelection()?.removeAllRanges();
    (e.target as HTMLElement).setPointerCapture(e.pointerId);
    setDragging(true);
    dragStartY.current = e.clientY;
    dragStartNorm.current = slider.norm;
    slider.beginGesture();
  }, [slider]);

  const onPointerMove = useCallback((e: React.PointerEvent) => {
    if (!dragging) return;
    e.preventDefault();
    window.getSelection()?.removeAllRanges();
    const dy = dragStartY.current - e.clientY;
    // 200px of drag = full normalised range; shift = fine (10x slower)
    const sensitivity = e.shiftKey ? 0.0005 : 0.005;
    const newNorm = dragStartNorm.current + dy * sensitivity;
    slider.setNorm(Math.max(0, Math.min(1, newNorm)));
  }, [dragging, slider]);

  const onPointerUp = useCallback((e: React.PointerEvent) => {
    if (!dragging) return;
    (e.target as HTMLElement).releasePointerCapture(e.pointerId);
    setDragging(false);
    slider.endGesture();
  }, [dragging, slider]);

  const onDoubleClick = () => {
    // Reset to mid-norm. (We don't currently know the param's actual default
    // value; mid is a reasonable behaviour and easy to extend later.)
    slider.beginGesture();
    slider.setNorm(0.5);
    slider.endGesture();
  };

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;
    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const sensitivity = e.shiftKey ? 0.001 : 0.005;
      slider.setNorm(slider.norm - Math.sign(e.deltaY) * sensitivity);
    };
    el.addEventListener("wheel", onWheel, { passive: false });
    return () => el.removeEventListener("wheel", onWheel);
  }, [slider]);

  const cx = size / 2;
  const cy = size / 2;
  const trackR     = size * 0.42;
  const indicatorR = size * 0.32;
  const angle = ARC_START + slider.norm * ARC_RANGE;

  const dot = polar(cx, cy, indicatorR, angle);
  const activePath = arcPath(cx, cy, trackR, ARC_START, angle);
  const fullPath   = arcPath(cx, cy, trackR, ARC_START, ARC_END);

  const glowAlpha = 0.55 + Math.min(0.45, energy * 0.6);
  const ringColor = `hsl(${hue}, 100%, 65%)`;
  const ringGlow  = `hsla(${hue}, 100%, 60%, ${glowAlpha})`;

  // If properties haven't arrived yet, fall back to a simple % indicator.
  // After they arrive, format(slider.value) shows real-world units.
  const readout = format
    ? format(slider.value)
    : (slider.properties ? slider.value.toFixed(2) : `${(slider.norm * 100).toFixed(0)}%`);

  return (
    <div className="cyber-knob" ref={containerRef} style={{ width: size }}>
      <div className="knob-label">{label}</div>
      <div
        className="knob-svg-wrap"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerCancel={onPointerUp}
        onDoubleClick={onDoubleClick}
        style={{ width: size, height: size }}
      >
        <svg width={size} height={size}>
          <defs>
            <radialGradient id={`hub-${paramId}`}>
              <stop offset="0%"  stopColor="#1a1426" />
              <stop offset="65%" stopColor="#0a0814" />
              <stop offset="100%" stopColor="#000" />
            </radialGradient>
            <filter id={`glow-${paramId}`} x="-50%" y="-50%" width="200%" height="200%">
              <feGaussianBlur stdDeviation={2 + energy * 4} result="blur" />
              <feMerge>
                <feMergeNode in="blur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
          </defs>

          <circle cx={cx} cy={cy} r={trackR + 4}
                  fill="none" stroke={ringGlow} strokeWidth={1}
                  opacity={0.3 + energy * 0.6} />

          <path d={fullPath} fill="none" stroke="#251c33"
                strokeWidth={3} strokeLinecap="round" />

          <path d={activePath} fill="none" stroke={ringColor}
                strokeWidth={3} strokeLinecap="round"
                filter={`url(#glow-${paramId})`} />

          <circle cx={cx} cy={cy} r={size * 0.26}
                  fill={`url(#hub-${paramId})`} stroke="#3a2f4d" strokeWidth={1} />

          <line x1={cx + (cx - dot.x) * -0.55} y1={cy + (cy - dot.y) * -0.55}
                x2={dot.x} y2={dot.y}
                stroke={ringColor} strokeWidth={2} strokeLinecap="round"
                filter={`url(#glow-${paramId})`} />

          <circle cx={dot.x} cy={dot.y} r={3.5}
                  fill={ringColor} filter={`url(#glow-${paramId})`} />
        </svg>
      </div>
      <div className="knob-readout">{readout}</div>
    </div>
  );
}
