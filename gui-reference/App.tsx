import React, { useEffect, useState } from 'react'
import Tonic from './components/Tonic'
export function App() {
  const [props, setProps] = useState({
    threshold: -20,
    ratio: 4,
    attack: 10,
    release: 100,
    gain: 12,
    timeMode: 'FIX' as const,
    meterSource: 'GR' as const,
    mix: 100,
    stereoMode: 'STEREO' as const,
    scHpf: 100,
    drive: 5,
    oversampling: 2 as const,
    externalSc: false,
    bypass: false,
  })
  const [meters, setMeters] = useState({
    inputLevel: 0,
    outputLevel: 0,
    gainReduction: 0,
    clipping: false,
  })
  // Simulate audio reactive meters
  useEffect(() => {
    let t = 0
    const interval = setInterval(() => {
      t += 0.1
      // Create a rhythmic pulsing input signal
      const baseSignal = Math.abs(
        Math.sin(t * 1.5) * 0.6 + Math.sin(t * 4.2) * 0.4,
      )
      // Add occasional peaks
      const peak = Math.random() > 0.95 ? Math.random() * 0.5 : 0
      const input = Math.min(1, baseSignal + peak)
      // Calculate fake compression based on threshold and ratio
      // Threshold is -60 to 0. Let's map it to 0-1 for simplicity
      const threshNorm = (props.threshold + 60) / 60
      let gr = 0
      if (input > threshNorm) {
        const overage = input - threshNorm
        gr = overage * (1 - 1 / props.ratio)
      }
      // Apply makeup gain
      const gainNorm = props.gain / 24 // 0 to 1
      const output = Math.min(1.1, (input - gr) * (1 + gainNorm * 0.5))
      // Clipping threshold
      const clip = output > 0.98
      setMeters({
        inputLevel: input,
        outputLevel: Math.min(1, output),
        gainReduction: gr * 2,
        clipping: clip,
      })
    }, 50)
    return () => clearInterval(interval)
  }, [props.threshold, props.ratio, props.gain])
  return (
    <div className="flex w-full min-h-screen justify-center items-center bg-[#050810] p-8">
      <Tonic
        {...props}
        {...meters}
        onParamChange={(p, v) =>
          setProps((prev) => ({
            ...prev,
            [p]: v,
          }))
        }
      />
    </div>
  )
}
