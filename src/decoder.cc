#include <emscripten.h>

#include "dsp.h"
#include "demodulator-wbfm.h"
#include "demodulator-nbfm.h"
#include "demodulator-am.h"
#include "demodulator-ssb.h"

double SAMPLERATE = 1024 * 1e3;
double AUDIO_SAMPLERATE = 48 * 1e3;

std::vector<uint8_t> in;
std::vector<float> out_left;
std::vector<float> out_right;

double cosine = 1, sine = 0;
Demodulator *demodulator;

extern "C" {
  EMSCRIPTEN_KEEPALIVE uint8_t *init_in(size_t bytes) {
    in.resize(bytes);
    return &in[0];
  }

  EMSCRIPTEN_KEEPALIVE float *get_left() {
    return &out_left[0];
  }

  EMSCRIPTEN_KEEPALIVE float *get_right() {
    return &out_right[0];
  }

  EMSCRIPTEN_KEEPALIVE void set_mode(int mode) {
    if (demodulator) delete demodulator;
    switch (mode)
    {
    case 0:
      demodulator = new Demodulator_WBFM(SAMPLERATE, AUDIO_SAMPLERATE);
      break;
    case 1:
      demodulator = new Demodulator_NBFM(SAMPLERATE, AUDIO_SAMPLERATE, 10000); // 12500
      break;
    case 2:
      demodulator = new Demodulator_AM(SAMPLERATE, AUDIO_SAMPLERATE, 6000);
      break;
    case 3:
      demodulator = new Demodulator_SSB(SAMPLERATE, AUDIO_SAMPLERATE, 2700, true);
      break;
    case 4:
      demodulator = new Demodulator_SSB(SAMPLERATE, AUDIO_SAMPLERATE, 2700, false);
      break;
    
    default:
      break;
    }
  }

  EMSCRIPTEN_KEEPALIVE size_t process(double freqOffset) {
    std::vector<float> IQ[2];
    iqSamplesFromUint8(in, IQ[0], IQ[1]);
    shiftFrequency(IQ, freqOffset, SAMPLERATE, cosine, sine);
    return demodulator->process(IQ[0], IQ[1], out_left, out_right);
  }
}