import { useState } from "react";
import { useLicense, openExternalUrl } from "../juceBridge";

const BUY_URL  = "https://carbonatedaudio.com/tonic";
const HELP_URL = "https://carbonatedaudio.com/faq";

// ─────────────────────────────────────────────────────────────────────
// ActivationOverlay
//
// Three states:
//   1. Activated (full plugin)            — overlay returns null
//   2. Demo build (BUILD_DEMO=ON)         — splash with TRY IT + BUY
//   3. Unactivated paid build             — full activation form
// ─────────────────────────────────────────────────────────────────────

export default function ActivationOverlay() {
  const license = useLicense();
  const [key, setKey] = useState("");
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState<{ ok: boolean; msg: string } | null>(null);
  const [demoDismissed, setDemoDismissed] = useState(false);

  // Activated → never show
  if (license.activated) return null;

  // Demo build, user already clicked TRY IT this session → don't re-block them
  if (license.isDemo && demoDismissed) return null;

  const onActivate = async () => {
    const trimmed = key.trim();
    if (trimmed.length < 8 || busy) return;
    setBusy(true);
    setStatus(null);
    try {
      const r = await license.activate(trimmed);
      setStatus({ ok: r.success, msg: r.message });
      if (r.success) setKey("");
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="activation-overlay">
      <div className="activation-scanlines" />

      <div className="activation-card">
        <div className="activation-corner tl" />
        <div className="activation-corner tr" />
        <div className="activation-corner bl" />
        <div className="activation-corner br" />

        <div className="activation-brand">
          <span className="activation-brand-dot" />
          <span className="activation-brand-name">TONIC</span>
          <span className="activation-brand-tag">CARBONATED AUDIO</span>
        </div>

        {license.isDemo ? (
          /* ============ DEMO SPLASH ============ */
          <>
            <div className="activation-demo-badge">DEMO</div>

            <h2 className="activation-title activation-title-demo">FREE DEMO</h2>
            <p className="activation-subtitle">
              full DSP — 60 seconds on, 10 seconds muted<br/>
              activate your license to unlock continuous audio
            </p>

            <button
              type="button"
              className="activation-button"
              onClick={() => setDemoDismissed(true)}
            >
              TRY IT
            </button>

            <button
              type="button"
              className="activation-button activation-button-secondary"
              onClick={() => openExternalUrl(BUY_URL)}
            >
              ▸ BUY TONIC — $20
            </button>

            <div className="activation-links">
              <button
                type="button"
                className="activation-link help"
                onClick={() => openExternalUrl(HELP_URL)}
              >
                NEED HELP?
              </button>
            </div>
          </>
        ) : (
          /* ============ FULL ACTIVATION FORM ============ */
          <>
            <h2 className="activation-title">ACTIVATE</h2>
            <p className="activation-subtitle">
              enter the serial number from your purchase confirmation email
            </p>

            <input
              type="text"
              className="activation-input"
              placeholder="XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX"
              value={key}
              onChange={(e) => setKey(e.target.value)}
              onKeyDown={(e) => { if (e.key === "Enter") onActivate(); }}
              disabled={busy}
              autoFocus
              spellCheck={false}
            />

            <button
              type="button"
              className={`activation-button ${busy ? "busy" : ""}`}
              onClick={onActivate}
              disabled={busy || key.trim().length < 8}
            >
              {busy ? "VERIFYING…" : "ACTIVATE"}
            </button>

            {status && (
              <div className={`activation-status ${status.ok ? "ok" : "err"}`}>
                {status.msg}
              </div>
            )}

            <div className="activation-links">
              <button
                type="button"
                className="activation-link buy"
                onClick={() => openExternalUrl(BUY_URL)}
              >
                ▸ BUY TONIC
              </button>
              <button
                type="button"
                className="activation-link help"
                onClick={() => openExternalUrl(HELP_URL)}
              >
                NEED HELP?
              </button>
            </div>
          </>
        )}

        <div className="activation-machine" title="machine identifier">
          MACHINE · {license.machineID.slice(0, 8) || "—"}
        </div>
      </div>
    </div>
  );
}
