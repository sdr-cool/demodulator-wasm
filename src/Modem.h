#pragma once

#include <vector>
#include <cstddef>
#include "utils.h"

class Modem {
public:
  virtual ~Modem() {}
  virtual size_t get_resample_size(size_t len) = 0;
  virtual size_t demodulate(const char *raw, size_t len, std::vector<float>& out) = 0;
};

class ModemAM : public Modem {
private:
  std::vector<float> iq_data;
  std::vector<float> demod_out;
  DcBlock dc;
  AudioResampler resampler;
  float aOutputCeil;
  float aOutputCeilMA;
  float aOutputCeilMAA;


public:
  ModemAM(size_t audio_samplerate, size_t samplerate):
    dc(25, 30.0f), resampler(audio_samplerate, samplerate),
    aOutputCeil(1), aOutputCeilMA(1), aOutputCeilMAA(1) {
  }

  ~ModemAM() {
  }

  virtual size_t get_resample_size(size_t len) {
    return resampler.get_resample_size(len);
  }

  virtual size_t demodulate(const char *raw, size_t len, std::vector<float>& out) {
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

    auto_gain(&demod_out[0], demod_out_len);
    size_t out_len = get_resample_size(demod_out_len);
    if (out.size() < out_len) out.resize(out_len);
    return resampler.resample(&demod_out[0], demod_out_len, &out[0]);  
  }

  void auto_gain(float *demod_out, const size_t len) {
    aOutputCeilMA = aOutputCeilMA + (aOutputCeil - aOutputCeilMA) * 0.025f;
    aOutputCeilMAA = aOutputCeilMAA + (aOutputCeilMA - aOutputCeilMAA) * 0.025f;
    aOutputCeil = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (demod_out[i] > aOutputCeil) {
            aOutputCeil = demod_out[i];
        }
    }
    
    float gain = 0.5f / aOutputCeilMAA;
    
    for (size_t i = 0; i < len; i++) {
        demod_out[i] *= gain;
    }
  }
};