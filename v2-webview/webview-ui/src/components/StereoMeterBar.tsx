interface StereoMeterBarProps {
  label: string;
  left: number;        // 0..1
  right: number;       // 0..1
  peakLeft?: number;
  peakRight?: number;
  height?: number;
}

// Two thin level bars side-by-side sharing one label. Used for IN and OUT
// columns. Identical signal on L/R → both bars track together. Stereo content
// or hard pan → bars diverge.
export default function StereoMeterBar({
  label, left, right, peakLeft, peakRight, height = 360
}: StereoMeterBarProps) {
  const lPct = Math.max(0, Math.min(1, left)) * 100;
  const rPct = Math.max(0, Math.min(1, right)) * 100;
  const lPk  = peakLeft  !== undefined ? Math.max(0, Math.min(1, peakLeft))  * 100 : null;
  const rPk  = peakRight !== undefined ? Math.max(0, Math.min(1, peakRight)) * 100 : null;

  return (
    <div className="stereo-meter">
      <div className="stereo-meter-label">{label}</div>
      <div className="stereo-meter-bars" style={{ height }}>
        <div className="stereo-meter-bar">
          <div className="stereo-meter-fill" style={{ height: `${lPct}%` }} />
          {lPk !== null && <div className="stereo-meter-peak" style={{ bottom: `${lPk}%` }} />}
        </div>
        <div className="stereo-meter-bar">
          <div className="stereo-meter-fill" style={{ height: `${rPct}%` }} />
          {rPk !== null && <div className="stereo-meter-peak" style={{ bottom: `${rPk}%` }} />}
        </div>
      </div>
      <div className="stereo-meter-channels">
        <span>L</span>
        <span>R</span>
      </div>
    </div>
  );
}
