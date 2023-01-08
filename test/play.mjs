import RtlSdr from 'rtlsdrjs'
import Decoder from './decode-worker.mjs'
import decoderWasm from '../src/demodulator.mjs'

const SAMPLE_RATE = 1024 * 1e3 // Must be a multiple of 512 * BUFS_PER_SEC
const BUFS_PER_SEC = 10
const SAMPLES_PER_BUF = Math.floor(SAMPLE_RATE / BUFS_PER_SEC)

function writeToStdout(left, right) {
  left = new Float32Array(left)
  right = new Float32Array(right)
  const out = new Float32Array(left.length * 2)
  for (let i = 0; i < left.length; i++) {
    out[i * 2] = left[i]
    out[i * 2 + 1] = right[i]
  }
  process.stdout.write(new Uint8Array(out.buffer))
}

async function play() {
  const sdr = await RtlSdr.requestDevice()
  await sdr.open({ ppm: 0.5 })
  await sdr.setSampleRate(SAMPLE_RATE)
  await sdr.setCenterFrequency(97.4 * 1e6)
  await sdr.resetBuffer()

  const decoder = new Decoder()
  decoder.setMode('FM');
  decoderWasm.setMode('FM');
  while (sdr) {
    const samples = await sdr.readSamples(SAMPLES_PER_BUF)
    setImmediate(() => {
      // const s = Date.now()
      if (process.env.WASM) {
        const [left, right] = decoderWasm.demodulate(samples)
        writeToStdout(left, right)
      } else {
        const [left, right] = decoder.process(samples, true, 0)
        writeToStdout(left, right)
      }
      // console.log(Date.now() - s)
    })
  }
}

play()
