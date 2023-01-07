#include <emscripten.h>

// #include <iostream>
#include <vector>
#include "Modem.h"

size_t SAMPLE_RATE = 1024 * 1e3;
size_t AUDIO_SAMPLE_RATE = 48 * 1e3;
Modem *g_modem = NULL;

std::vector<unsigned char> g_in;
std::vector<float> g_out;

extern "C" {
  EMSCRIPTEN_KEEPALIVE void set_audio_samplerate(size_t audio_sample_rate) {
    AUDIO_SAMPLE_RATE = audio_sample_rate;
  }

  EMSCRIPTEN_KEEPALIVE void set_samplerate(size_t sample_rate) {
    SAMPLE_RATE = sample_rate;
  }

  EMSCRIPTEN_KEEPALIVE void set_mode(int mode) {
    if (g_modem) delete g_modem;
    switch (mode)
    {
    case 1:
      g_modem = new ModemAM(AUDIO_SAMPLE_RATE, SAMPLE_RATE);
      break;
    case 2:
      g_modem = new ModemNFM(AUDIO_SAMPLE_RATE, SAMPLE_RATE);
      break;

    default:
      break;
    }
  }

  EMSCRIPTEN_KEEPALIVE void *init_in(size_t bytes_len) {
    if (g_in.size() < bytes_len) g_in.resize(bytes_len);
    return &g_in[0];
  }

  EMSCRIPTEN_KEEPALIVE void *get_out_ptr() {
    return &g_out[0];
  }

  EMSCRIPTEN_KEEPALIVE size_t demodulate() {
    return g_modem->demodulate(&g_in[0], g_in.size(), g_out);
  }
}