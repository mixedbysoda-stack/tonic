
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
    "./components/**/*.{js,ts,jsx,tsx}",
    "./pages/**/*.{js,ts,jsx,tsx}",
    "./*.{js,ts,jsx,tsx}"
  ],
  theme: {
    extend: {
      colors: {
        darkroom: '#0A0F1F',
        brass: '#C9A876',
        ivory: '#F5EFE0',
        fluoyellow: '#D4FF00',
        eviolet: '#7B2FFF',
        tubeamber: '#FFB347',
        clipping: '#FF2D7E',
      },
      fontFamily: {
        retro: ['Orbitron', 'sans-serif'],
        mono: ['Space Mono', 'monospace'],
        sans: ['Inter', 'sans-serif'],
      },
      boxShadow: {
        'glow-violet': '0 0 20px 5px rgba(123, 47, 255, 0.4)',
        'glow-yellow': '0 0 15px 2px rgba(212, 255, 0, 0.6)',
        'glow-amber': '0 0 15px 2px rgba(255, 179, 71, 0.6)',
      }
    },
  },
  plugins: [],
}
