import { default  as demodulator } from './demodulator_wasm.js'

function bufferToPtr(buffer) {
  const ptr = demodulator._createPtr(buffer.byteLength)
  new Uint8Array(demodulator.HEAP8.buffer, ptr).set(new Uint8Array(buffer))
  return ptr;
}

const modeMap = { 'AM': 1 }
export default {
  setMode(mode) {
    demodulator._set_mode(modeMap[mode])
  },

  demodulate(buffer) {
    const ptr = bufferToPtr(buffer)
    const out = demodulator._createPtr(1024 * 24);
    const outLen = demodulator._demodulate(ptr, buffer.byteLength, out)

    const outBuffer = new ArrayBuffer(outLen * 4)
    new Uint8Array(outBuffer).set(new Uint8Array(demodulator.HEAP8.buffer, out, outLen * 4))

    demodulator._freePtr(out)
    demodulator._freePtr(ptr)
    return outBuffer
  }
}