interface MeterBarProps {
  label: string;
  value: number;            // 0..1
  peak?: number;            // 0..1, for peak-hold tick
  variant?: "level" | "gr"; // gr = inverted (fills downward from top)
  height?: number;
}

export default function MeterBar({ label, value, peak, variant = "level", height = 160 }: MeterBarProps) {
  const v = Math.max(0, Math.min(1, value)) * 100;
  const p = peak !== undefined ? Math.max(0, Math.min(1, peak)) * 100 : null;

  return (
    <div className="meter-bar-container">
      <div className="meter-bar-label">{label}</div>
      <div className="meter-bar" style={{ height }}>
        <div className="meter-bar-track">
          {variant === "level" ? (
            <>
              <div className="meter-bar-fill level" style={{ height: `${v}%` }} />
              {p !== null && (
                <div className="meter-bar-peak" style={{ bottom: `${p}%` }} />
              )}
            </>
          ) : (
            <div className="meter-bar-fill gr" style={{ height: `${v}%` }} />
          )}
        </div>
      </div>
      <div className="meter-bar-value">
        {(value * 100).toFixed(0)}
      </div>
    </div>
  );
}
