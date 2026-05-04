import { useToggle } from "../juceBridge";

interface BoolToggleProps {
  paramId: string;
  label: string;
  glyph?: string;     // optional left-side icon glyph
  variant?: "default" | "warning";   // BYPASS uses warning variant
}

export default function BoolToggle({ paramId, label, glyph, variant = "default" }: BoolToggleProps) {
  const toggle = useToggle(paramId);

  return (
    <button
      className={`bool-toggle ${toggle.on ? "on" : "off"} variant-${variant}`}
      onClick={() => toggle.toggle()}
      type="button"
    >
      {glyph && <span className="bool-glyph">{glyph}</span>}
      <span className="bool-label">{label}</span>
      <span className="bool-led" />
    </button>
  );
}
