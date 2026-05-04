import React, { useEffect, useState, useRef, memo } from 'react'
import { motion, useAnimation, AnimatePresence } from 'framer-motion'
export type TonicProps = {
  threshold: number
  ratio: number
  attack: number
  release: number
  gain: number
  timeMode: 'FIX' | 'MAN' | 'FIX+MAN'
  meterSource: 'INPUT' | 'OUTPUT' | 'GR'
  mix: number
  stereoMode: 'STEREO' | 'DUAL_MONO' | 'M_S'
  scHpf: number
  drive: number
  oversampling: 1 | 2 | 4
  externalSc: boolean
  bypass: boolean
  inputLevel: number
  outputLevel: number
  gainReduction: number
  clipping: boolean
  onParamChange: (param: string, value: any) => void
}
// --- Subcomponents ---
const Knob = ({
  label,
  value,
  min,
  max,
  onChange,
  format = (v: number) => v.toFixed(1),
  size = 'large',
}: {
  label: string
  value: number
  min: number
  max: number
  onChange: (val: number) => void
  format?: (v: number) => string
  size?: 'large' | 'small'
}) => {
  const [isDragging, setIsDragging] = useState(false)
  const startY = useRef(0)
  const startVal = useRef(0)
  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true)
    startY.current = e.clientY
    startVal.current = value
  }
  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isDragging) return
      const deltaY = startY.current - e.clientY
      const range = max - min
      const sensitivity = 0.005 // Adjust for drag speed
      let newVal = startVal.current + deltaY * range * sensitivity
      newVal = Math.max(min, Math.min(max, newVal))
      onChange(newVal)
    }
    const handleMouseUp = () => {
      setIsDragging(false)
    }
    if (isDragging) {
      window.addEventListener('mousemove', handleMouseMove)
      window.addEventListener('mouseup', handleMouseUp)
    }
    return () => {
      window.removeEventListener('mousemove', handleMouseMove)
      window.removeEventListener('mouseup', handleMouseUp)
    }
  }, [isDragging, min, max, onChange])
  // Map value to angle (-135 to 135 degrees)
  const angle = -135 + ((value - min) / (max - min)) * 270
  const dim = size === 'large' ? 72 : 40
  return (
    <div className="flex flex-col items-center select-none">
      <div
        className={`text-ivory font-sans font-semibold tracking-widest mb-2 ${size === 'large' ? 'text-xs' : 'text-[10px]'}`}
      >
        {label}
      </div>
      <div
        className="relative rounded-full cursor-ns-resize"
        style={{
          width: dim,
          height: dim,
          background:
            'radial-gradient(circle at 30% 30%, #E8D0A5, #C9A876, #8A6D42)',
          boxShadow:
            'inset 0 0 10px rgba(0,0,0,0.5), 0 5px 10px rgba(0,0,0,0.5)',
          border: '2px solid #5A4D32',
        }}
        onMouseDown={handleMouseDown}
      >
        {/* Pointer */}
        <div
          className="absolute top-0 left-0 w-full h-full"
          style={{
            transform: `rotate(${angle}deg)`,
          }}
        >
          <div className="absolute top-1 left-1/2 -translate-x-1/2 w-1.5 h-1/4 bg-ivory rounded-full shadow-sm" />
        </div>
      </div>
      <div
        className={`text-fluoyellow font-mono mt-2 ${size === 'large' ? 'text-sm' : 'text-xs'}`}
        style={{
          textShadow: '0 0 5px rgba(212,255,0,0.5)',
        }}
      >
        {format(value)}
      </div>
    </div>
  )
}
const Toggle = ({
  label,
  value,
  options,
  onChange,
}: {
  label?: string
  value: string | number | boolean
  options: {
    label: string
    value: any
  }[]
  onChange: (val: any) => void
}) => {
  const currentIndex = options.findIndex((o) => o.value === value)
  const handleClick = () => {
    const nextIndex = (currentIndex + 1) % options.length
    onChange(options[nextIndex].value)
  }
  // Map index to angle for rocker switch (-30, 0, 30) or (-30, 30)
  const angle =
    options.length === 3
      ? currentIndex === 0
        ? -30
        : currentIndex === 1
          ? 0
          : 30
      : currentIndex === 0
        ? -30
        : 30
  return (
    <div
      className="flex flex-col items-center select-none cursor-pointer"
      onClick={handleClick}
    >
      {label && (
        <div className="text-ivory font-sans font-semibold tracking-widest mb-2 text-[10px]">
          {label}
        </div>
      )}
      <div className="relative w-8 h-12 bg-[#1A1A1A] rounded-sm border border-[#333] flex items-center justify-center shadow-inner">
        <motion.div
          className="w-6 h-8 bg-gradient-to-b from-gray-300 to-gray-500 rounded-sm shadow-md border border-gray-400"
          animate={{
            rotateX: angle,
          }}
          transition={{
            type: 'spring',
            stiffness: 300,
            damping: 20,
          }}
          style={{
            transformOrigin: 'center',
          }}
        >
          <div className="w-full h-full bg-gradient-to-b from-transparent via-white/20 to-black/20" />
        </motion.div>
      </div>
      <div className="text-ivory font-sans text-[9px] mt-1 tracking-wider h-3">
        {options[currentIndex].label}
      </div>
    </div>
  )
}
const Tube = ({ gain, drive }: { gain: number; drive: number }) => {
  // gain: 0 to 20 -> opacity 0.4 to 1
  const glowOpacity = 0.4 + (gain / 20) * 0.6
  // drive: 0 to 10 -> color shift from amber to orange/red
  const r = 255
  const g = Math.max(100, 179 - (drive / 10) * 79)
  const b = Math.max(0, 71 - (drive / 10) * 71)
  const color = `rgb(${r},${g},${b})`
  return (
    <div className="relative w-16 h-48 flex justify-center items-end pb-4">
      {/* Glow behind */}
      <motion.div
        className="absolute bottom-8 w-12 h-24 rounded-full blur-xl"
        style={{
          backgroundColor: color,
        }}
        animate={{
          opacity: glowOpacity,
        }}
        transition={{
          duration: 0.2,
        }}
      />

      <svg
        viewBox="0 0 60 160"
        className="w-full h-full relative z-10 drop-shadow-xl"
      >
        {/* Pins */}
        <rect x="15" y="150" width="4" height="10" fill="#888" />
        <rect x="25" y="150" width="4" height="10" fill="#888" />
        <rect x="35" y="150" width="4" height="10" fill="#888" />
        <rect x="45" y="150" width="4" height="10" fill="#888" />

        {/* Base */}
        <path d="M 10 140 L 50 140 L 55 150 L 5 150 Z" fill="#222" />

        {/* Glass Envelope */}
        <path
          d="M 10 140 L 10 40 C 10 10, 50 10, 50 40 L 50 140 Z"
          fill="rgba(255,255,255,0.05)"
          stroke="rgba(245, 239, 224, 0.3)"
          strokeWidth="2"
        />

        {/* Internal Structure */}
        <rect x="20" y="50" width="20" height="80" fill="#333" rx="2" />
        <line x1="30" y1="50" x2="30" y2="30" stroke="#555" strokeWidth="2" />

        {/* Filament */}
        <motion.path
          d="M 25 130 L 25 60 L 30 55 L 35 60 L 35 130"
          fill="none"
          stroke={color}
          strokeWidth="2"
          animate={{
            opacity: glowOpacity,
            filter: `drop-shadow(0 0 5px ${color})`,
          }}
        />

        {/* Glass highlight */}
        <path
          d="M 15 40 C 15 20, 30 15, 30 15"
          fill="none"
          stroke="rgba(255,255,255,0.4)"
          strokeWidth="3"
          strokeLinecap="round"
        />
      </svg>
    </div>
  )
}
const Tumbler = ({
  outputLevel,
  gainReduction,
}: {
  outputLevel: number
  gainReduction: number
}) => {
  // outputLevel 0-1 -> height 0-100
  const targetHeight = outputLevel * 100
  // Bubbles
  const [bubbles, setBubbles] = useState<
    {
      id: number
      x: number
      size: number
      speed: number
    }[]
  >([])
  const bubbleId = useRef(0)
  useEffect(() => {
    if (gainReduction < 0.05) return
    const density = Math.floor(gainReduction * 10)
    const interval = setInterval(
      () => {
        setBubbles((prev) => {
          const newBubbles = [...prev]
          for (let i = 0; i < density; i++) {
            newBubbles.push({
              id: bubbleId.current++,
              x: 25 + Math.random() * 50,
              size: 1 + Math.random() * 3,
              speed: 1 + gainReduction * 3 + Math.random() * 2,
            })
          }
          // Keep only recent bubbles to avoid memory leak
          return newBubbles.slice(-50)
        })
      },
      100 - gainReduction * 80,
    )
    return () => clearInterval(interval)
  }, [gainReduction])
  const showFoam = gainReduction > 0.7
  const overflow = gainReduction > 0.9
  return (
    <div className="relative w-20 h-28 flex justify-center items-end -translate-x-3 translate-y-3">
      {/* Violet rim light glow behind */}
      <div className="absolute inset-0 bg-eviolet opacity-20 blur-2xl rounded-full" />

      <svg
        viewBox="0 0 100 150"
        className="w-full h-full relative z-10 overflow-visible"
      >
        {/* Back glass */}
        <path
          d="M 20 10 L 20 140 C 20 145, 80 145, 80 140 L 80 10"
          fill="none"
          stroke="rgba(245, 239, 224, 0.1)"
          strokeWidth="2"
        />

        {/* Liquid */}
        <motion.rect
          x="22"
          width="56"
          fill="#D4FF00"
          opacity="0.85"
          initial={{
            y: 140,
            height: 0,
          }}
          animate={{
            y: 140 - targetHeight,
            height: targetHeight,
          }}
          transition={{
            type: 'spring',
            stiffness: 100,
            damping: 20,
          }}
          style={{
            filter: 'drop-shadow(0 0 15px rgba(212,255,0,0.8))',
          }}
        />

        {/* Bubbles */}
        <AnimatePresence>
          {bubbles.map((b) => (
            <motion.circle
              key={b.id}
              cx={b.x}
              r={b.size}
              fill="#FFF"
              opacity="0.6"
              initial={{
                cy: 140,
              }}
              animate={{
                cy: 140 - targetHeight - 10,
              }}
              exit={{
                opacity: 0,
              }}
              transition={{
                duration: 2 / b.speed,
                ease: 'linear',
              }}
            />
          ))}
        </AnimatePresence>

        {/* Foam */}
        <AnimatePresence>
          {showFoam && (
            <motion.path
              initial={{
                opacity: 0,
              }}
              animate={{
                opacity: 1,
                y: 140 - targetHeight,
              }}
              exit={{
                opacity: 0,
              }}
              d="M 22 0 Q 30 -5, 36 0 T 50 0 T 64 0 T 78 0 L 78 5 L 22 5 Z"
              fill="#FFF"
              opacity="0.9"
            />
          )}
        </AnimatePresence>

        {/* Overflow */}
        <AnimatePresence>
          {overflow && (
            <motion.path
              initial={{
                opacity: 0,
                pathLength: 0,
              }}
              animate={{
                opacity: 1,
                pathLength: 1,
              }}
              exit={{
                opacity: 0,
              }}
              d="M 20 10 Q 15 15, 15 30 M 80 10 Q 85 15, 85 30"
              fill="none"
              stroke="#D4FF00"
              strokeWidth="3"
              strokeLinecap="round"
              style={{
                filter: 'drop-shadow(0 0 5px #D4FF00)',
              }}
            />
          )}
        </AnimatePresence>

        {/* Front glass */}
        <path
          d="M 15 5 L 15 140 C 15 150, 85 150, 85 140 L 85 5"
          fill="none"
          stroke="rgba(245, 239, 224, 0.4)"
          strokeWidth="3"
        />
        <path
          d="M 25 15 C 25 10, 35 8, 35 8"
          fill="none"
          stroke="rgba(255,255,255,0.5)"
          strokeWidth="2"
          strokeLinecap="round"
        />
      </svg>
    </div>
  )
}
export default function Tonic(props: TonicProps) {
  const controls = useAnimation()
  useEffect(() => {
    if (props.clipping) {
      controls.start({
        x: [0, -2, 2, -1, 1, 0],
        transition: {
          duration: 0.2,
        },
      })
    }
  }, [props.clipping, controls])
  // Halo pulse based on input level
  const haloOpacity = 0.3 + props.inputLevel * 0.5
  const haloScale = 1 + props.inputLevel * 0.05
  return (
    <motion.div
      className="relative w-[900px] h-[560px] bg-[#121623] rounded-xl border-4 border-[#2A2F42] shadow-2xl overflow-hidden flex flex-col"
      animate={controls}
    >
      {/* UV Halo Background */}
      <motion.div
        className="absolute inset-0 pointer-events-none"
        style={{
          background:
            'radial-gradient(circle at 50% 50%, rgba(123, 47, 255, 0.15) 0%, transparent 70%)',
          boxShadow: 'inset 0 0 50px rgba(123, 47, 255, 0.2)',
        }}
        animate={{
          opacity: haloOpacity,
          scale: haloScale,
        }}
        transition={{
          type: 'tween',
          ease: 'easeOut',
          duration: 0.1,
        }}
      />

      {/* Clipping Flash Overlay */}
      <AnimatePresence>
        {props.clipping && (
          <motion.div
            className="absolute inset-0 bg-clipping pointer-events-none z-50 mix-blend-screen"
            initial={{
              opacity: 0.5,
            }}
            animate={{
              opacity: 0,
            }}
            exit={{
              opacity: 0,
            }}
            transition={{
              duration: 0.15,
            }}
          />
        )}
      </AnimatePresence>

      {/* Front Panel Texture */}
      <div
        className="absolute inset-0 opacity-5 pointer-events-none"
        style={{
          backgroundImage:
            "url(\"data:image/svg+xml,%3Csvg width='100' height='100' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100' height='100' filter='url(%23noise)'/%3E%3C/svg%3E\")",
        }}
      />

      {/* Top Bar */}
      <div className="flex justify-between items-center px-8 py-4 border-b border-[#2A2F42] bg-[#0A0F1F]/80 z-10">
        <div className="flex items-baseline space-x-4">
          <h1
            className="font-retro text-4xl text-fluoyellow tracking-wider"
            style={{
              textShadow:
                '0 0 15px rgba(123, 47, 255, 0.8), 0 0 5px rgba(212, 255, 0, 0.5)',
            }}
          >
            TONIC
          </h1>
          <span className="font-sans text-ivory/70 text-sm tracking-widest uppercase">
            opto-tube vocal compressor
          </span>
        </div>
        <div className="flex items-center space-x-6">
          <select className="bg-[#1A1A1A] text-ivory border border-[#333] rounded px-3 py-1 font-sans text-sm focus:outline-none focus:border-eviolet">
            <option>Default</option>
            <option>Vocal Magic</option>
            <option>Crushed Drum Bus</option>
            <option>Smooth Bass</option>
          </select>
          <Toggle
            value={props.bypass}
            options={[
              {
                label: 'IN',
                value: false,
              },
              {
                label: 'BYP',
                value: true,
              },
            ]}
            onChange={(v) => props.onParamChange('bypass', v)}
          />
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col justify-between p-8 z-10 relative">
        {/* Top Row: Tubes + Knobs */}
        <div className="flex justify-between items-center w-full">
          <Tube gain={props.gain} drive={props.drive} />

          <div className="flex space-x-12">
            <Knob
              label="THRESHOLD"
              value={props.threshold}
              min={-60}
              max={0}
              onChange={(v) => props.onParamChange('threshold', v)}
              format={(v) => `${v.toFixed(1)} dB`}
            />
            <Knob
              label="RATIO"
              value={props.ratio}
              min={1}
              max={20}
              onChange={(v) => props.onParamChange('ratio', v)}
              format={(v) => `${v.toFixed(1)}:1`}
            />
            <Knob
              label="ATTACK"
              value={props.attack}
              min={0.1}
              max={100}
              onChange={(v) => props.onParamChange('attack', v)}
              format={(v) => `${v.toFixed(1)} ms`}
            />
            <Knob
              label="RELEASE"
              value={props.release}
              min={10}
              max={2000}
              onChange={(v) => props.onParamChange('release', v)}
              format={(v) => `${v.toFixed(0)} ms`}
            />
            <Knob
              label="GAIN"
              value={props.gain}
              min={0}
              max={24}
              onChange={(v) => props.onParamChange('gain', v)}
              format={(v) => `+${v.toFixed(1)} dB`}
            />
          </div>

          <Tube gain={props.gain} drive={props.drive} />
        </div>

        {/* Middle Row: Switches + Tumbler */}
        <div className="flex justify-center items-center space-x-16 -mt-8">
          <Toggle
            label="TIME MODE"
            value={props.timeMode}
            options={[
              {
                label: 'FIX',
                value: 'FIX',
              },
              {
                label: 'MAN',
                value: 'MAN',
              },
              {
                label: 'F+M',
                value: 'FIX+MAN',
              },
            ]}
            onChange={(v) => props.onParamChange('timeMode', v)}
          />

          <div className="flex justify-center items-center">
            <Tumbler
              outputLevel={props.outputLevel}
              gainReduction={props.gainReduction}
            />
          </div>

          <Toggle
            label="METER"
            value={props.meterSource}
            options={[
              {
                label: 'IN',
                value: 'INPUT',
              },
              {
                label: 'GR',
                value: 'GR',
              },
              {
                label: 'OUT',
                value: 'OUTPUT',
              },
            ]}
            onChange={(v) => props.onParamChange('meterSource', v)}
          />
        </div>
      </div>

      {/* Bottom QoL Strip */}
      <div className="bg-[#1A1A1A]/80 border-t border-[#2A2F42] px-8 py-3 flex justify-between items-center z-10">
        <Knob
          size="small"
          label="MIX"
          value={props.mix}
          min={0}
          max={100}
          onChange={(v) => props.onParamChange('mix', v)}
          format={(v) => `${v.toFixed(0)}%`}
        />
        <Toggle
          label="STEREO"
          value={props.stereoMode}
          options={[
            {
              label: 'ST',
              value: 'STEREO',
            },
            {
              label: 'DUAL',
              value: 'DUAL_MONO',
            },
            {
              label: 'M/S',
              value: 'M_S',
            },
          ]}
          onChange={(v) => props.onParamChange('stereoMode', v)}
        />
        <Knob
          size="small"
          label="SC HPF"
          value={props.scHpf}
          min={20}
          max={500}
          onChange={(v) => props.onParamChange('scHpf', v)}
          format={(v) => `${v.toFixed(0)} Hz`}
        />
        <Knob
          size="small"
          label="DRIVE"
          value={props.drive}
          min={0}
          max={10}
          onChange={(v) => props.onParamChange('drive', v)}
          format={(v) => v.toFixed(1)}
        />
        <Toggle
          label="OVERSAMPLE"
          value={props.oversampling}
          options={[
            {
              label: '1x',
              value: 1,
            },
            {
              label: '2x',
              value: 2,
            },
            {
              label: '4x',
              value: 4,
            },
          ]}
          onChange={(v) => props.onParamChange('oversampling', v)}
        />
        <Toggle
          label="SIDECHAIN"
          value={props.externalSc}
          options={[
            {
              label: 'INT',
              value: false,
            },
            {
              label: 'EXT',
              value: true,
            },
          ]}
          onChange={(v) => props.onParamChange('externalSc', v)}
        />
      </div>
    </motion.div>
  )
}
