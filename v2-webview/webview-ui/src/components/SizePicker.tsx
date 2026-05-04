import { useSize } from "../juceBridge";

// SizePicker — discrete editor-scale toggle that lives in the header.
//
// Free-form drag resize is unreliable in some hosts (notably JUCE's
// Standalone wrapper, which doesn't honour the bounds constrainer), so
// instead the user picks one of four explicit sizes. Each option calls
// back to the C++ setEditorSize native function.
//
// Labels (per user spec): Large 100% / Medium 75% / Small 50% / Mini 35%.
// Short tags shown in the UI: L · M · S · XS.

const TAGS: Record<string, string> = {
  Large:  "L",
  Medium: "M",
  Small:  "S",
  Mini:   "XS",
};

export default function SizePicker() {
  const { width, sizes, setSize } = useSize();

  if (sizes.length === 0) return null;

  return (
    <div className="size-picker" role="group" aria-label="Editor size">
      <span className="size-picker-label">SIZE</span>
      {sizes.map((s) => {
        const active = s.width === width;
        const tag = TAGS[s.name] ?? s.name.charAt(0);
        return (
          <button
            key={s.name}
            type="button"
            className={`size-picker-button ${active ? "active" : ""}`}
            title={`${s.name} — ${s.width}×${s.height}`}
            onClick={() => { if (!active) setSize(s.width, s.height); }}
          >
            {tag}
          </button>
        );
      })}
    </div>
  );
}
