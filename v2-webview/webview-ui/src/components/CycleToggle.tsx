import { useCombo } from "../juceBridge";

interface CycleToggleProps {
  paramId: string;
  label: string;
  // Fallback choices used until JUCE's propertiesChanged event arrives.
  // Should match the AudioParameterChoice's StringArray on the C++ side.
  choices: string[];
}

export default function CycleToggle({ paramId, label, choices: fallback }: CycleToggleProps) {
  const combo = useCombo(paramId, fallback);
  const choices = combo.choices.length > 0 ? combo.choices : fallback;
  const safeIdx = Math.max(0, Math.min(choices.length - 1, combo.index));

  return (
    <div className="cycle-toggle" onClick={() => combo.cycle()}>
      <div className="cycle-label">{label}</div>
      <div className="cycle-window">
        {choices.map((c, i) => (
          <span key={c} className={`cycle-choice ${i === safeIdx ? "active" : ""}`}>
            {c}
          </span>
        ))}
      </div>
    </div>
  );
}
