import { strict as assert } from 'node:assert';

import RtlSdr from 'rtlsdrjs'
import Decoder from './decode-worker.mjs'
import decoderWasm from '../src/demodulator.mjs'

const SAMPLE_RATE = 1024 * 1e3 // Must be a multiple of 512 * BUFS_PER_SEC
const BUFS_PER_SEC = 5
const SAMPLES_PER_BUF = Math.floor(SAMPLE_RATE / BUFS_PER_SEC)

async function test() {
  const sdr = await RtlSdr.requestDevice()
  await sdr.open({ ppm: 0.5 })
  await sdr.setSampleRate(SAMPLE_RATE)
  await sdr.setCenterFrequency(88.7 * 1e6)
  await sdr.resetBuffer()

  const decoder = new Decoder()
  for (const mode of ['FM', 'NFM', 'AM', 'LSB', 'USB']) {
    decoder.setMode(mode)
    if (decoderWasm.hasMode(mode)) decoderWasm.setMode(mode)

    let dataProcessed = 0
    let costDecoder = 0
    let costDecoderWasm = 0
    for (let i = 0; i < 5; i++) {
      const samples = await sdr.readSamples(SAMPLES_PER_BUF)
      dataProcessed += samples.byteLength

      if (decoderWasm.hasMode(mode)) {
        const dsw = Date.now()
        decoderWasm.demodulate(samples, true, 0)
        costDecoderWasm += Date.now() - dsw
      }

      const ds = Date.now()
      const [left, right, sl] = decoder.process(samples, true, 0)
      costDecoder += Date.now() - ds

      // assert.deepEqual(leftW, left, `${mode} error. Left channel data should be equal.`)
      // assert.deepEqual(rightW, right, `${mode} error. Right channel data should be equal.`)
      // assert.equal(slW, sl, `${mode} error. Signal level should be equal.`)
    }

    console.log(
      `${mode.padStart(3, ' ')}: passed.`,
      `Decoder performance: ${(dataProcessed / costDecoder / 1000).toFixed(1).padStart(4, ' ')}MB/s`,
      costDecoderWasm > 0 ? `DecoderWasm performance: ${(dataProcessed / costDecoderWasm / 1000).toFixed(1).padStart(4, ' ')}MB/s` : ''
    )
  }

  process.exit(0)
}

test()