// Tonic v2 — JUCE 8 native-integration bridge.
// Verified against the actual JUCE 8.0.12 source:
//   modules/juce_gui_extra/detail/juce_WebControlRelayEvents.h
//   modules/juce_gui_extra/misc/juce_WebControlRelays.cpp
//   modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp
//
// Protocol summary (all event objects are FLAT — value is not nested):
//
// Slider event "__juce__slider{id}":
//   C++ → JS:
//     { eventType: "valueChanged",      value: <float 0..1 normalised> }
//     { eventType: "propertiesChanged", start, end, skew, name, label, numSteps, interval, parameterIndex }
//   JS → C++:
//     { eventType: "valueChanged",        value: <float 0..1> }
//     { eventType: "sliderDragStarted" }
//     { eventType: "sliderDragEnded" }
//     { eventType: "requestInitialUpdate" }
//
// ComboBox event "__juce__comboBox{id}":
//   C++ → JS:
//     { eventType: "valueChanged",      value: <float 0..1 normalised> }
//     { eventType: "propertiesChanged", name, parameterIndex, choices: ["a","b","c"] }
//   JS → C++:
//     { eventType: "valueChanged",        value: <float 0..1 normalised> }
//     { eventType: "requestInitialUpdate" }
//   IMPORTANT: 0..1 maps to 0..(numChoices-1). For 3 choices: 0=0.0, 1=0.5, 2=1.0.
//
// Toggle event "__juce__toggle{id}":
//   C++ → JS:
//     { eventType: "valueChanged",      value: <bool> }
//     { eventType: "propertiesChanged", name, parameterIndex }
//   JS → C++:
//     { eventType: "valueChanged",        value: <bool> }
//     { eventType: "requestInitialUpdate" }

import { useEffect, useRef, useState } from "react";

type Backend = NonNullable<typeof window.__JUCE__>["backend"];

let cachedBackend: Backend | null = null;
const backendCallbacks = new Set<(b: Backend) => void>();

export function onBackendReady(cb: (b: Backend) => void) {
  if (cachedBackend) {
    cb(cachedBackend);
    return;
  }
  backendCallbacks.add(cb);
  const tick = () => {
    const b = window.__JUCE__?.backend;
    if (b) {
      cachedBackend = b;
      backendCallbacks.forEach((fn) => fn(b));
      backendCallbacks.clear();
    } else {
      setTimeout(tick, 30);
    }
  };
  tick();
}

// =====================================================================
// Slider
// =====================================================================

export interface SliderProperties {
  start: number;
  end: number;
  skew: number;
  name: string;
  label: string;
  numSteps: number;
  interval: number;
  parameterIndex: number;
}

export interface SliderState {
  value: number;                            // REAL-WORLD value (e.g. -20 for threshold)
  norm: number;                             // 0..1 derived from value + range + skew, for arc rendering
  properties: SliderProperties | null;      // populated after first propertiesChanged
  setValue: (realWorld: number) => void;    // takes a real-world value
  setNorm: (norm: number) => void;          // takes 0..1, converts to real-world via skew
  beginGesture: () => void;
  endGesture: () => void;
}

// JUCE NormalisableRange skew transforms — exact mirror of juce_NormalisableRange.h
function realToNorm(real: number, start: number, end: number, skew: number): number {
  if (end === start) return 0;
  const proportion = Math.max(0, Math.min(1, (real - start) / (end - start)));
  return skew === 1 ? proportion : Math.pow(proportion, skew);
}
function normToReal(norm: number, start: number, end: number, skew: number): number {
  const clamped = Math.max(0, Math.min(1, norm));
  const proportion = skew === 1 ? clamped : Math.pow(clamped, 1 / skew);
  return start + (end - start) * proportion;
}

export function useSlider(id: string): SliderState {
  const eventId = `__juce__slider${id}`;
  // value is the REAL-WORLD value sent by JUCE. Initialised to NaN so we can tell
  // if propertiesChanged has set a sane default before the user interacts.
  const [value, setValueLocal] = useState<number>(0);
  const [properties, setProperties] = useState<SliderProperties | null>(null);
  const backendRef = useRef<Backend | null>(null);
  const propertiesRef = useRef<SliderProperties | null>(null);
  propertiesRef.current = properties;

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      backendRef.current = backend;
      listenerId = backend.addEventListener(eventId, (raw) => {
        const p = raw as { eventType?: string; value?: number } & Partial<SliderProperties>;
        if (p.eventType === "valueChanged" && typeof p.value === "number") {
          setValueLocal(p.value);                                 // REAL-WORLD value
        } else if (p.eventType === "propertiesChanged") {
          setProperties({
            start: p.start ?? 0,
            end: p.end ?? 1,
            skew: p.skew ?? 1,
            name: p.name ?? "",
            label: p.label ?? "",
            numSteps: p.numSteps ?? 0,
            interval: p.interval ?? 0,
            parameterIndex: p.parameterIndex ?? -1,
          });
        }
      });
      backend.emitEvent(eventId, { eventType: "requestInitialUpdate" });
    });
    return () => {
      if (listenerId !== null && backendRef.current) {
        backendRef.current.removeEventListener(eventId, listenerId);
      }
    };
  }, [eventId]);

  const setValue = (realWorld: number) => {
    const props = propertiesRef.current;
    let clamped = realWorld;
    if (props) {
      const lo = Math.min(props.start, props.end);
      const hi = Math.max(props.start, props.end);
      clamped = Math.max(lo, Math.min(hi, realWorld));
    }
    setValueLocal(clamped);
    backendRef.current?.emitEvent(eventId, { eventType: "valueChanged", value: clamped });
  };

  const setNorm = (norm: number) => {
    const props = propertiesRef.current;
    if (!props) {
      // No properties yet — we can't convert. Skip.
      return;
    }
    const real = normToReal(norm, props.start, props.end, props.skew);
    setValue(real);
  };

  const beginGesture = () => {
    backendRef.current?.emitEvent(eventId, { eventType: "sliderDragStarted" });
  };
  const endGesture = () => {
    backendRef.current?.emitEvent(eventId, { eventType: "sliderDragEnded" });
  };

  // Derived norm for arc rendering
  const norm = properties
    ? realToNorm(value, properties.start, properties.end, properties.skew)
    : 0;

  return { value, norm, properties, setValue, setNorm, beginGesture, endGesture };
}

// =====================================================================
// ComboBox (used for cycle toggles)
// =====================================================================
//
// Tricky: JUCE expects the JS side to send a 0..1 *normalised* value.
// For a choice param with N choices, the mapping is:
//   index → norm: i / (N - 1)        (or 0 if N == 1)
//   norm → index: round(norm * (N - 1))
// So the hook needs to know N. It reads N from the propertiesChanged event
// (which carries `choices: string[]`), but you can pass `fallbackChoices` as
// a default so the UI can render before propertiesChanged arrives.

export interface ComboState {
  index: number;
  choices: string[];
  setIndex: (i: number) => void;
  cycle: () => void;
}

export function useCombo(id: string, fallbackChoices: string[] = []): ComboState {
  const eventId = `__juce__comboBox${id}`;
  const [index, setIndexLocal] = useState(0);
  const [choices, setChoices] = useState<string[]>(fallbackChoices);
  const choicesRef = useRef<string[]>(fallbackChoices);
  const backendRef = useRef<Backend | null>(null);
  choicesRef.current = choices;

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      backendRef.current = backend;
      listenerId = backend.addEventListener(eventId, (raw) => {
        const p = raw as { eventType?: string; value?: number; choices?: string[] };
        if (p.eventType === "valueChanged" && typeof p.value === "number") {
          const n = choicesRef.current.length;
          const idx = n > 1 ? Math.round(p.value * (n - 1)) : 0;
          setIndexLocal(Math.max(0, Math.min(n - 1, idx)));
        } else if (p.eventType === "propertiesChanged" && Array.isArray(p.choices)) {
          if (p.choices.length > 0) setChoices(p.choices);
        }
      });
      backend.emitEvent(eventId, { eventType: "requestInitialUpdate" });
    });
    return () => {
      if (listenerId !== null && backendRef.current) {
        backendRef.current.removeEventListener(eventId, listenerId);
      }
    };
  }, [eventId]);

  const setIndex = (i: number) => {
    const n = choicesRef.current.length;
    const clamped = Math.max(0, Math.min(n - 1, Math.round(i)));
    setIndexLocal(clamped);
    const norm = n > 1 ? clamped / (n - 1) : 0;
    backendRef.current?.emitEvent(eventId, { eventType: "valueChanged", value: norm });
  };

  const cycle = () => setIndex((index + 1) % Math.max(1, choices.length));

  return { index, choices, setIndex, cycle };
}

// =====================================================================
// Toggle
// =====================================================================

export interface ToggleState {
  on: boolean;
  setOn: (v: boolean) => void;
  toggle: () => void;
}

export function useToggle(id: string): ToggleState {
  const eventId = `__juce__toggle${id}`;
  const [on, setOnLocal] = useState(false);
  const backendRef = useRef<Backend | null>(null);

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      backendRef.current = backend;
      listenerId = backend.addEventListener(eventId, (raw) => {
        const p = raw as { eventType?: string; value?: boolean };
        if (p.eventType === "valueChanged" && typeof p.value === "boolean") {
          setOnLocal(p.value);
        }
        // propertiesChanged carries no useful state for toggles.
      });
      backend.emitEvent(eventId, { eventType: "requestInitialUpdate" });
    });
    return () => {
      if (listenerId !== null && backendRef.current) {
        backendRef.current.removeEventListener(eventId, listenerId);
      }
    };
  }, [eventId]);

  const setOn = (v: boolean) => {
    setOnLocal(v);
    backendRef.current?.emitEvent(eventId, { eventType: "valueChanged", value: v });
  };
  const toggle = () => setOn(!on);

  return { on, setOn, toggle };
}

// =====================================================================
// Meters (custom event, pushed at 30Hz from the C++ Timer)
// =====================================================================

export interface MeterState {
  input: number;
  output: number;
  inputL: number;
  inputR: number;
  outputL: number;
  outputR: number;
  gainReductionDb: number;
  clipping: boolean;
  gr01: number;
  inputPeak: number;
  outputPeak: number;
  inputPeakL: number;
  inputPeakR: number;
  outputPeakL: number;
  outputPeakR: number;
}

export function useMeters(): MeterState {
  const [meters, setMeters] = useState<MeterState>({
    input: 0, output: 0,
    inputL: 0, inputR: 0, outputL: 0, outputR: 0,
    gainReductionDb: 0, clipping: false,
    gr01: 0,
    inputPeak: 0, outputPeak: 0,
    inputPeakL: 0, inputPeakR: 0, outputPeakL: 0, outputPeakR: 0,
  });
  const peakRef = useRef({ in: 0, out: 0, inL: 0, inR: 0, outL: 0, outR: 0 });

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      listenerId = backend.addEventListener("tonicMeters", (raw) => {
        const p = raw as Partial<MeterState>;
        const input = p.input ?? 0;
        const output = p.output ?? 0;
        const inputL = p.inputL ?? input;
        const inputR = p.inputR ?? input;
        const outputL = p.outputL ?? output;
        const outputR = p.outputR ?? output;
        const gainReductionDb = p.gainReductionDb ?? 0;
        const clipping = p.clipping ?? false;

        const decay = 0.97;
        peakRef.current.in   = Math.max(input,   peakRef.current.in   * decay);
        peakRef.current.out  = Math.max(output,  peakRef.current.out  * decay);
        peakRef.current.inL  = Math.max(inputL,  peakRef.current.inL  * decay);
        peakRef.current.inR  = Math.max(inputR,  peakRef.current.inR  * decay);
        peakRef.current.outL = Math.max(outputL, peakRef.current.outL * decay);
        peakRef.current.outR = Math.max(outputR, peakRef.current.outR * decay);

        setMeters({
          input, output,
          inputL, inputR, outputL, outputR,
          gainReductionDb,
          clipping,
          gr01: Math.max(0, Math.min(1, gainReductionDb / 24)),
          inputPeak:  peakRef.current.in,
          outputPeak: peakRef.current.out,
          inputPeakL: peakRef.current.inL,
          inputPeakR: peakRef.current.inR,
          outputPeakL: peakRef.current.outL,
          outputPeakR: peakRef.current.outR,
        });
      });
    });
    return () => {
      if (listenerId !== null && cachedBackend) {
        cachedBackend.removeEventListener("tonicMeters", listenerId);
      }
    };
  }, []);

  return meters;
}

export function useBridgeReady(): boolean {
  const [ready, setReady] = useState(false);
  useEffect(() => {
    onBackendReady(() => setReady(true));
  }, []);
  return ready;
}

// =====================================================================
// Native function invocation (JS → C++)
//
// Re-implements JUCE's getNativeFunction helper directly on top of the
// backend.emitEvent protocol so we don't have to bundle their JS module.
//
// Protocol (from juce_gui_extra/native/javascript/index.js):
//   1. JS emits "__juce__invoke" { name, params, resultId } where resultId
//      is a unique counter so concurrent calls don't collide.
//   2. C++ runs the registered NativeFunction and emits "__juce__complete"
//      { promiseId, result }.
//   3. JS resolves the promise associated with that promiseId.
// =====================================================================

const pendingInvokes = new Map<number, (result: unknown) => void>();
let nextPromiseId = 0;
let invokeListenerInstalled = false;

function ensureInvokeListenerInstalled() {
  if (invokeListenerInstalled) return;
  onBackendReady((backend) => {
    backend.addEventListener("__juce__complete", (raw) => {
      const p = raw as { promiseId?: number; result?: unknown };
      if (typeof p.promiseId !== "number") return;
      const resolver = pendingInvokes.get(p.promiseId);
      if (resolver) {
        resolver(p.result);
        pendingInvokes.delete(p.promiseId);
      }
    });
    invokeListenerInstalled = true;
  });
}

export function invokeNative(name: string, ...args: unknown[]): Promise<unknown> {
  ensureInvokeListenerInstalled();
  return new Promise((resolve) => {
    const promiseId = nextPromiseId++;
    pendingInvokes.set(promiseId, resolve);
    onBackendReady((backend) => {
      backend.emitEvent("__juce__invoke", {
        name,
        params: args,
        resultId: promiseId,
      });
    });
  });
}

// =====================================================================
// Presets — listens for tonicPresets events from C++ and exposes
// load/save/delete via invokeNative.
// =====================================================================

export interface PresetEntry {
  name: string;
  source: "factory" | "user";
}

export interface PresetState {
  presets: PresetEntry[];
  current: string;
  dirty: boolean;
  load:   (name: string) => Promise<boolean>;
  save:   (name: string) => Promise<boolean>;
  remove: (name: string) => Promise<boolean>;
  refresh: () => Promise<void>;
}

export function usePresets(): PresetState {
  const [state, setState] = useState<{ presets: PresetEntry[]; current: string; dirty: boolean }>({
    presets: [],
    current: "",
    dirty: false,
  });

  const apply = (raw: unknown) => {
    const p = raw as { presets?: PresetEntry[]; current?: string; dirty?: boolean };
    if (!Array.isArray(p?.presets)) return;
    setState({
      presets: p.presets,
      current: p.current ?? "",
      dirty:   !!p.dirty,
    });
  };

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      listenerId = backend.addEventListener("tonicPresets", apply);
      // Pull initial state — the constructor also emits at +250ms but this
      // covers the case where mount happens later.
      invokeNative("listPresets").then(apply);
    });
    return () => {
      if (listenerId !== null && cachedBackend) {
        cachedBackend.removeEventListener("tonicPresets", listenerId);
      }
    };
  }, []);

  const refresh = async () => {
    const r = await invokeNative("listPresets");
    apply(r);
  };

  const load = async (name: string) => {
    const ok = await invokeNative("loadPreset", name);
    return Boolean(ok);
  };
  const save = async (name: string) => {
    const ok = await invokeNative("savePreset", name);
    return Boolean(ok);
  };
  const remove = async (name: string) => {
    const ok = await invokeNative("deletePreset", name);
    return Boolean(ok);
  };

  return { ...state, load, save, remove, refresh };
}

// =====================================================================
// Editor size — discrete scale presets exposed by C++ via setEditorSize /
// getEditorSize. The React SizePicker calls setSize(width, height).
// =====================================================================

export interface SizePreset {
  name: string;
  width: number;
  height: number;
}

export interface SizeState {
  width: number;
  height: number;
  sizes: SizePreset[];
  setSize: (width: number, height: number) => Promise<boolean>;
}

export function useSize(): SizeState {
  const [state, setState] = useState<{ width: number; height: number; sizes: SizePreset[] }>({
    width: 1080,
    height: 640,
    sizes: [],
  });

  const apply = (raw: unknown) => {
    const p = raw as Partial<SizeState>;
    if (typeof p?.width !== "number" || typeof p?.height !== "number") return;
    setState({
      width: p.width,
      height: p.height,
      sizes: Array.isArray(p.sizes) ? p.sizes : [],
    });
  };

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      listenerId = backend.addEventListener("tonicSize", apply);
      invokeNative("getEditorSize").then(apply);
    });
    return () => {
      if (listenerId !== null && cachedBackend) {
        cachedBackend.removeEventListener("tonicSize", listenerId);
      }
    };
  }, []);

  const setSize = async (width: number, height: number) => {
    const ok = await invokeNative("setEditorSize", width, height);
    return Boolean(ok);
  };

  return { ...state, setSize };
}

// =====================================================================
// License — drives the React activation overlay. C++ exposes:
//   getLicenseState() → { activated, machineID }
//   activate(key)     → { success, message }     (deferred Promise — returns
//                                                  when the network round-trip
//                                                  to /activate-tonic finishes)
// and emits "tonicLicense" whenever activation state flips.
// =====================================================================

export interface LicenseState {
  activated: boolean;
  machineID: string;
  isDemo: boolean;
}

export interface LicenseHook extends LicenseState {
  activate: (key: string) => Promise<{ success: boolean; message: string }>;
  refresh: () => Promise<void>;
}

// Open an https or mailto URL in the user's default browser / mail client.
// Hands off to juce::URL::launchInDefaultBrowser on the C++ side so we don't
// try to navigate the embedded webview itself.
export async function openExternalUrl(url: string): Promise<boolean> {
  const ok = await invokeNative("openExternalUrl", url);
  return Boolean(ok);
}

export function useLicense(): LicenseHook {
  const [state, setState] = useState<LicenseState>({ activated: true, machineID: "", isDemo: false });

  const apply = (raw: unknown) => {
    const p = raw as Partial<LicenseState>;
    if (typeof p?.activated !== "boolean") return;
    setState({
      activated: p.activated,
      machineID: p.machineID ?? "",
      isDemo:    Boolean(p.isDemo),
    });
  };

  useEffect(() => {
    let listenerId: number | null = null;
    onBackendReady((backend) => {
      listenerId = backend.addEventListener("tonicLicense", apply);
      invokeNative("getLicenseState").then(apply);
    });
    return () => {
      if (listenerId !== null && cachedBackend) {
        cachedBackend.removeEventListener("tonicLicense", listenerId);
      }
    };
  }, []);

  const activate = async (key: string) => {
    const r = await invokeNative("activate", key);
    const p = r as { success?: boolean; message?: string };
    return {
      success: Boolean(p?.success),
      message: p?.message ?? "",
    };
  };

  const refresh = async () => {
    const r = await invokeNative("getLicenseState");
    apply(r);
  };

  return { ...state, activate, refresh };
}
