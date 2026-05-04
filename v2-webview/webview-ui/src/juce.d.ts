// Minimal type shim for the JUCE 8 frontend bridge that's auto-injected
// by WebBrowserComponent when withNativeIntegrationEnabled() is set.
//
// Native function invocation is built on top of `backend.emitEvent` — see
// `invokeNative()` in juceBridge.ts. JUCE's own `getNativeFunction` helper
// lives in their `./juce` JS module which we don't bundle; we re-implement
// the same protocol ourselves so we stay TypeScript-only.
declare global {
  interface Window {
    __JUCE__?: {
      backend: {
        emitEvent: (eventId: string, payload: unknown) => void;
        addEventListener: (eventId: string, fn: (payload: unknown) => void) => number;
        removeEventListener: (eventId: string, listenerId: number) => void;
      };
      initialisationData: Record<string, unknown[]>;
    };
  }
}

export {};
