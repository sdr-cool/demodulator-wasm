import { default  as demodulator } from './demodulator_wasm.js'

function bufferToPtr(buffer) {
  const ptr = demodulator._init_in(buffer.byteLength)
  new Uint8Array(demodulator.HEAP8.buffer, ptr).set(new Uint8Array(buffer))
  return ptr
}

function ptrToBuffer(ptr, bytes) {
  const out = new ArrayBuffer(bytes)
  new Uint8Array(out).set(new Uint8Array(demodulator.HEAP8.buffer, ptr, bytes))
  return out
}

const modeMap = { 'FM': 0, 'NFM': 1, 'AM': 2, 'USB': 3, 'LSB': 4 }
export default {
  setMode(mode) {
    demodulator._set_mode(modeMap[mode])
  },

  hasMode(mode) {
    return modeMap[mode] >= 0
  },

  demodulate(buffer, freqOffset = 0) {
    bufferToPtr(buffer)
    const outLen = demodulator._process(freqOffset)

    const left = ptrToBuffer(demodulator._get_left(), outLen * 4)
    const right = ptrToBuffer(demodulator._get_right(), outLen * 4)

    return [left, right]
  }
}