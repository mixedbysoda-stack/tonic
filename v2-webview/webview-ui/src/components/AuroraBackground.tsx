import { useEffect, useRef } from "react";

// Cheap canvas Aurora — multiple soft moving blobs that breathe with audio.
// No external deps; rendered with ctx2d + radial gradients.
//
// Sits behind the entire chassis as ambient atmosphere. Subtle by design —
// the particle storm in the hero is the loud element, this is the wash.

interface AuroraProps {
  intensity: number;     // 0..1 — speed + brightness boost
}

export default function AuroraBackground({ intensity }: AuroraProps) {
  const ref = useRef<HTMLCanvasElement>(null);
  const intRef = useRef(intensity);
  intRef.current = intensity;

  useEffect(() => {
    const canvas = ref.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d")!;
    const dpr = window.devicePixelRatio || 1;

    const resize = () => {
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width * dpr;
      canvas.height = rect.height * dpr;
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.scale(dpr, dpr);
    };
    resize();

    const blobs = [
      { x: 0.2, y: 0.3, hue: 270, baseR: 380, phase: 0    },
      { x: 0.7, y: 0.4, hue: 305, baseR: 440, phase: 1.2  },
      { x: 0.4, y: 0.8, hue: 250, baseR: 360, phase: 2.7  },
      { x: 0.85, y: 0.85, hue: 60, baseR: 240, phase: 4.1 },   // a hint of fluo-yellow corner
    ];

    let raf = 0;
    let t0 = performance.now();
    const loop = (t: number) => {
      const dt = (t - t0) / 1000;
      const r = canvas.getBoundingClientRect();
      ctx.clearRect(0, 0, r.width, r.height);

      ctx.globalCompositeOperation = "lighter";
      const energy = intRef.current;

      for (const b of blobs) {
        const drift = 0.04 + energy * 0.08;
        const cx = (b.x + Math.sin(dt * drift + b.phase) * 0.06) * r.width;
        const cy = (b.y + Math.cos(dt * drift + b.phase * 0.7) * 0.06) * r.height;
        const radius = b.baseR * (1 + energy * 0.3);
        const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
        const baseAlpha = b.hue === 60 ? 0.04 : 0.10;
        const alpha = baseAlpha + energy * 0.08;
        grad.addColorStop(0, `hsla(${b.hue}, 100%, 60%, ${alpha})`);
        grad.addColorStop(1, `hsla(${b.hue}, 100%, 60%, 0)`);
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, r.width, r.height);
      }

      ctx.globalCompositeOperation = "source-over";
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);

    window.addEventListener("resize", resize);
    return () => {
      cancelAnimationFrame(raf);
      window.removeEventListener("resize", resize);
    };
  }, []);

  return <canvas ref={ref} className="aurora-bg" />;
}
