import { useEffect, useRef } from "react";

interface ParticleFieldProps {
  // 0..1 — drives speed + density
  intensity: number;
  // 0..1 — drives hue toward red (compression heat)
  heat: number;
  // optional clipping flash trigger
  clipping?: boolean;
}

interface Particle {
  x: number; y: number;
  vx: number; vy: number;
  r: number;
  baseHue: number;
  life: number;
}

export default function ParticleField({ intensity, heat, clipping }: ParticleFieldProps) {
  const ref = useRef<HTMLCanvasElement>(null);
  const intensityRef = useRef(intensity);
  const heatRef = useRef(heat);
  const clipFlashRef = useRef(0);

  intensityRef.current = intensity;
  heatRef.current = heat;
  if (clipping) clipFlashRef.current = 1;

  useEffect(() => {
    const canvas = ref.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d", { alpha: true })!;
    const dpr = window.devicePixelRatio || 1;

    const resize = () => {
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width * dpr;
      canvas.height = rect.height * dpr;
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.scale(dpr, dpr);
    };
    resize();

    const particles: Particle[] = [];
    const rect = canvas.getBoundingClientRect();
    const targetCount = 140;
    for (let i = 0; i < targetCount; i++) {
      particles.push({
        x: Math.random() * rect.width,
        y: Math.random() * rect.height,
        vx: (Math.random() - 0.5) * 0.5,
        vy: (Math.random() - 0.5) * 0.5,
        r: Math.random() * 1.4 + 0.4,
        baseHue: 270 + Math.random() * 60,    // violet → magenta
        life: Math.random(),
      });
    }

    let raf = 0;
    let lastT = performance.now();
    const loop = (t: number) => {
      const dt = (t - lastT) / 1000;
      lastT = t;
      const r = canvas.getBoundingClientRect();
      ctx.clearRect(0, 0, r.width, r.height);

      const inten = intensityRef.current;
      const h = heatRef.current;
      const energy = 0.3 + inten * 1.8;

      // Soft radial vignette
      const grad = ctx.createRadialGradient(r.width / 2, r.height / 2, 0,
                                             r.width / 2, r.height / 2, Math.max(r.width, r.height) * 0.6);
      grad.addColorStop(0, `rgba(168, 85, 247, ${0.06 + inten * 0.18})`);
      grad.addColorStop(1, "rgba(0,0,0,0)");
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, r.width, r.height);

      ctx.globalCompositeOperation = "lighter";

      for (const p of particles) {
        // Drift outward from center when energy is high (explosive feel)
        const dx = p.x - r.width / 2;
        const dy = p.y - r.height / 2;
        const dist = Math.hypot(dx, dy) || 1;
        const radialPush = (inten * 0.4) / dist;

        p.x += (p.vx * energy + dx * radialPush) * dt * 60;
        p.y += (p.vy * energy + dy * radialPush) * dt * 60;
        p.life += dt * (0.3 + inten * 0.7);

        // Wrap around
        if (p.x < 0) p.x += r.width;
        if (p.x > r.width) p.x -= r.width;
        if (p.y < 0) p.y += r.height;
        if (p.y > r.height) p.y -= r.height;

        // Hue shifts toward red as compression "heat" rises
        const hue = p.baseHue - h * 270;       // violet (270) → orange/red (~0)
        const sat = 100;
        const lit = 55 + Math.sin(p.life * 4) * 15;
        const alpha = 0.55 + inten * 0.45;

        ctx.fillStyle = `hsla(${(hue + 360) % 360}, ${sat}%, ${lit}%, ${alpha})`;
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.r * (1 + inten * 1.2), 0, Math.PI * 2);
        ctx.fill();
      }

      ctx.globalCompositeOperation = "source-over";

      // Clipping flash — a magenta scanline sweeping down
      if (clipFlashRef.current > 0.02) {
        const y = (1 - clipFlashRef.current) * r.height;
        const flashGrad = ctx.createLinearGradient(0, y - 30, 0, y + 30);
        flashGrad.addColorStop(0, "rgba(255, 45, 146, 0)");
        flashGrad.addColorStop(0.5, `rgba(255, 45, 146, ${clipFlashRef.current * 0.8})`);
        flashGrad.addColorStop(1, "rgba(255, 45, 146, 0)");
        ctx.fillStyle = flashGrad;
        ctx.fillRect(0, y - 30, r.width, 60);
        clipFlashRef.current *= 0.92;
      }

      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);

    window.addEventListener("resize", resize);
    return () => {
      cancelAnimationFrame(raf);
      window.removeEventListener("resize", resize);
    };
  }, []);

  return <canvas ref={ref} className="particle-canvas" />;
}
