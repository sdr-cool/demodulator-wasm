#include <emscripten.h>

// #include <iostream>
#include <vector>
#include "Modem.h"

size_t SAMPLE_RATE = 1024 * 1e3;
size_t AUDIO_SAMPLE_RATE = 48 * 1e3;
Modem *g_modem = NULL;

extern "C" {
  EMSCRIPTEN_KEEPALIVE void *createPtr(size_t len) {
    return malloc(len);
  }

  EMSCRIPTEN_KEEPALIVE void freePtr(void *ptr) {
    return free(ptr);
  }

  EMSCRIPTEN_KEEPALIVE void set_audio_samplerate(size_t audio_sample_rate) {
    AUDIO_SAMPLE_RATE = audio_sample_rate;
  }

  EMSCRIPTEN_KEEPALIVE void set_samplerate(size_t sample_rate) {
    SAMPLE_RATE = sample_rate;
  }

  EMSCRIPTEN_KEEPALIVE void set_mode(int mode) {
    switch (mode)
    {
    case 1:
      g_modem = new ModemAM(AUDIO_SAMPLE_RATE, SAMPLE_RATE);
      break;  
    default:
      break;
    }
  }

  EMSCRIPTEN_KEEPALIVE size_t demodulate(const char *raw, size_t len, float *out) {
    return g_modem->demodulate(raw, len, out);
  }
}