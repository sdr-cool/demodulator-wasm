#pragma once

#include <vector>
#include <cstddef>
#include "utils.h"

class Modem {
public:
  virtual size_t demodulate(const char *raw, size_t len, float *out) = 0;
};

class ModemAM : public Modem {
private:
  std::vector<float> iq_data;
  std::vector<float> demod_out;
  DcBlock dc;
  AudioResampler resampler;

public:
  ModemAM(size_t audio_samplerate, size_t samplerate): dc(25, 30.0f), resampler(audio_samplerate, samplerate) {
  }

  ~ModemAM() {
  }

  virtual size_t demodulate(const char *raw, size_t len, float *out) {
    if (iq_data.size() < len) iq_data.resize(len);
    fill_iq(raw, &iq_data[0], len);

    size_t demod_out_len = len / 2;
    if (demod_out.size() < demod_out_len) demod_out.resize(demod_out_len);
    
    for (size_t idx = 0; idx < len; idx += 2) {
      const float I = iq_data[idx];
      const float Q = iq_data[idx + 1];
      dc.push(sqrt(I * I + Q * Q));
      dc.execute(&demod_out[idx / 2]);
    }

    return resampler.resample(&demod_out[0], demod_out_len, out);  
  }
};