import { spawn } from 'child_process'
import RtlSdr from 'rtlsdrjs'
import Decoder from './decode-worker.mjs'

const SAMPLE_RATE = 1024 * 1e3 // Must be a multiple of 512 * BUFS_PER_SEC
const BUFS_PER_SEC = 1
const SAMPLES_PER_BUF = Math.floor(SAMPLE_RATE / BUFS_PER_SEC)

async function play() {
  const sdr = await RtlSdr.requestDevice()
  await sdr.open({ ppm: 0.5 })
  await sdr.setSampleRate(SAMPLE_RATE)
  await sdr.setCenterFrequency(97.4 * 1e6)
  await sdr.resetBuffer()

  const decoder = new Decoder()
  let ts = Date.now()
  while (sdr) {
    const samples = await sdr.readSamples(SAMPLES_PER_BUF)
    setImmediate(() => {
      let [left, right, sl] = decoder.process(samples, true, 0)
      left = new Float32Array(left)
      right = new Float32Array(right)
      const out = new Float32Array(left.length * 2)
      for (let i = 0; i < left.length; i++) {
        out[i * 2] = left[i]
        out[i * 2 + 1] = right[i]
      }
      process.stdout.write(new Uint8Array(out.buffer))
    })
  }
}

play()
