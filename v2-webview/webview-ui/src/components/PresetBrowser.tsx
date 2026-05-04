import { useEffect, useMemo, useRef, useState } from "react";
import { usePresets, type PresetEntry } from "../juceBridge";

// ─────────────────────────────────────────────────────────────────────
// PresetBrowser
//
// Header-resident dropdown: shows current preset name, prev/next chevrons,
// click-to-open menu with Factory + User groups, search filter, save and
// delete actions. Cyberpunk styling matches the existing CyberKnob /
// CycleToggle aesthetic.
// ─────────────────────────────────────────────────────────────────────

export default function PresetBrowser() {
  const { presets, current, dirty, load, save, remove } = usePresets();

  const [open, setOpen] = useState(false);
  const [filter, setFilter] = useState("");
  const [savePromptOpen, setSavePromptOpen] = useState(false);
  const [saveName, setSaveName] = useState("");
  const popoverRef = useRef<HTMLDivElement>(null);
  const triggerRef = useRef<HTMLButtonElement>(null);

  // Click outside to close
  useEffect(() => {
    if (!open && !savePromptOpen) return;
    const onDown = (e: MouseEvent) => {
      const t = e.target as Node;
      if (popoverRef.current && !popoverRef.current.contains(t)
          && triggerRef.current && !triggerRef.current.contains(t)) {
        setOpen(false);
        setSavePromptOpen(false);
      }
    };
    document.addEventListener("mousedown", onDown);
    return () => document.removeEventListener("mousedown", onDown);
  }, [open, savePromptOpen]);

  const grouped = useMemo(() => {
    const f = filter.trim().toLowerCase();
    const match = (p: PresetEntry) => !f || p.name.toLowerCase().includes(f);
    return {
      factory: presets.filter((p) => p.source === "factory" && match(p)),
      user:    presets.filter((p) => p.source === "user"    && match(p)),
    };
  }, [presets, filter]);

  // Derive prev/next for chevron navigation. Order = factory then user
  // (matches the dropdown), so chevrons step through the same flat list.
  const ordered = useMemo(
    () => [...presets.filter(p => p.source === "factory"),
           ...presets.filter(p => p.source === "user")],
    [presets]
  );

  const currentIndex = ordered.findIndex((p) => p.name === current);

  const stepBy = async (delta: number) => {
    if (ordered.length === 0) return;
    const next = currentIndex < 0
      ? (delta > 0 ? 0 : ordered.length - 1)
      : (currentIndex + delta + ordered.length) % ordered.length;
    await load(ordered[next].name);
  };

  const onSaveSubmit = async () => {
    const trimmed = saveName.trim();
    if (!trimmed) return;
    const ok = await save(trimmed);
    if (ok) {
      setSaveName("");
      setSavePromptOpen(false);
      setOpen(false);
    }
  };

  const display = current
    ? `${current}${dirty ? " *" : ""}`
    : (dirty ? "(unsaved)" : "—");

  return (
    <div className="preset-browser">
      <button
        type="button"
        className="preset-arrow"
        aria-label="Previous preset"
        onClick={() => stepBy(-1)}
      >
        ◀
      </button>

      <button
        ref={triggerRef}
        type="button"
        className={`preset-current ${open ? "open" : ""} ${dirty ? "dirty" : ""}`}
        onClick={() => { setOpen(o => !o); setSavePromptOpen(false); }}
      >
        <span className="preset-current-label">PRESET</span>
        <span className="preset-current-name" title={display}>{display}</span>
        <span className="preset-chevron">{open ? "▴" : "▾"}</span>
      </button>

      <button
        type="button"
        className="preset-arrow"
        aria-label="Next preset"
        onClick={() => stepBy(1)}
      >
        ▶
      </button>

      {open && (
        <div ref={popoverRef} className="preset-popover">
          <div className="preset-search-row">
            <input
              type="text"
              className="preset-search"
              placeholder="search…"
              value={filter}
              onChange={(e) => setFilter(e.target.value)}
              autoFocus
            />
            <button
              type="button"
              className="preset-save-toggle"
              onClick={() => { setSavePromptOpen(s => !s); setSaveName(current || ""); }}
            >
              + SAVE AS
            </button>
          </div>

          {savePromptOpen && (
            <div className="preset-save-row">
              <input
                type="text"
                className="preset-save-input"
                placeholder="preset name…"
                value={saveName}
                onChange={(e) => setSaveName(e.target.value)}
                onKeyDown={(e) => { if (e.key === "Enter") onSaveSubmit(); }}
                autoFocus
              />
              <button type="button" className="preset-save-confirm" onClick={onSaveSubmit}>
                SAVE
              </button>
            </div>
          )}

          <div className="preset-list">
            {grouped.factory.length > 0 && (
              <>
                <div className="preset-group-header">FACTORY</div>
                {grouped.factory.map((p) => (
                  <PresetRow key={`f-${p.name}`}
                             preset={p}
                             active={p.name === current}
                             onLoad={() => { load(p.name).then(ok => { if (ok) setOpen(false); }); }}
                  />
                ))}
              </>
            )}

            {grouped.user.length > 0 ? (
              <>
                <div className="preset-group-header">USER</div>
                {grouped.user.map((p) => (
                  <PresetRow key={`u-${p.name}`}
                             preset={p}
                             active={p.name === current}
                             onLoad={() => { load(p.name).then(ok => { if (ok) setOpen(false); }); }}
                             onDelete={() => remove(p.name)}
                  />
                ))}
              </>
            ) : (
              <div className="preset-empty">no user presets — save one to start</div>
            )}
          </div>
        </div>
      )}
    </div>
  );
}

interface PresetRowProps {
  preset: PresetEntry;
  active: boolean;
  onLoad: () => void;
  onDelete?: () => void;
}

function PresetRow({ preset, active, onLoad, onDelete }: PresetRowProps) {
  return (
    <div className={`preset-row ${active ? "active" : ""}`}>
      <button type="button" className="preset-row-load" onClick={onLoad}>
        <span className="preset-row-name">{preset.name}</span>
      </button>
      {onDelete && (
        <button
          type="button"
          className="preset-row-delete"
          onClick={(e) => { e.stopPropagation(); onDelete(); }}
          title="Delete"
        >
          ✕
        </button>
      )}
    </div>
  );
}
