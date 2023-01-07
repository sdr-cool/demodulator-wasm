#pragma once

#include <vector>
#include <cstddef>
#include "utils.h"

class Modem {
private:
  float aOutputCeil;
  float aOutputCeilMA;
  float aOutputCeilMAA;

protected:
  std::vector<liquid_float_complex> iq_data;
  std::vector<float> demod_out;

public:
  Modem() : aOutputCeil(1), aOutputCeilMA(1), aOutputCeilMAA(1) {}
  virtual ~Modem() {}
  virtual size_t get_resample_size(size_t len) = 0;
  virtual size_t demodulate(const unsigned char *raw, size_t len, std::vector<float>& out) = 0;

protected:
  void init_input(const unsigned char *raw, size_t len) {
    iq_data.resize(len / 2);
    for (size_t idx = 0; idx < len; idx += 2) {
      iq_data[idx / 2].real = float(raw[idx]) / 128 - 0.995;
      iq_data[idx / 2].imag = float(raw[idx + 1]) / 128 - 0.995;
    }
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

class ModemAM : public Modem {
private:
  DcBlock dc;
  AudioResampler resampler;

public:
  ModemAM(size_t audio_samplerate, size_t samplerate): dc(25, 30.0f), resampler(audio_samplerate, samplerate) {
  }

  ~ModemAM() {
  }

  virtual size_t get_resample_size(size_t len) {
    return resampler.get_resample_size(len);
  }

  virtual size_t demodulate(const unsigned char *raw, size_t len, std::vector<float>& out) {
    init_input(raw, len);
    demod_out.resize(iq_data.size());

    for (size_t idx = 0; idx < iq_data.size(); idx++) {
      const float I = iq_data[idx].real;
      const float Q = iq_data[idx].imag;
      dc.push(sqrt(I * I + Q * Q));
      dc.execute(&demod_out[idx]);
    }

    auto_gain(&demod_out[0], demod_out.size());
    size_t out_len = get_resample_size(demod_out.size());
    if (out.size() < out_len) out.resize(out_len);
    return resampler.resample(&demod_out[0], demod_out.size(), &out[0]);
  }
};

class ModemNFM : public Modem {
private:
  freqdem fm_dem;
  AudioResampler resampler;

public:
  ModemNFM(size_t audio_samplerate, size_t samplerate): resampler(audio_samplerate, samplerate) {
    fm_dem = freqdem_create(0.5f);
  }

  ~ModemNFM() {
    freqdem_destroy(fm_dem);
  }

  virtual size_t get_resample_size(size_t len) {
    return resampler.get_resample_size(len);
  }

  virtual size_t demodulate(const unsigned char *raw, size_t len, std::vector<float>& out) {
    init_input(raw, len);
    demod_out.resize(iq_data.size());

    int ret = freqdem_demodulate_block(fm_dem, &iq_data[0], iq_data.size(), &demod_out[0]);
    
    // auto_gain(&demod_out[0], demod_out.size());
    size_t out_len = get_resample_size(demod_out.size());
    if (out.size() < out_len) out.resize(out_len);
    return resampler.resample(&demod_out[0], demod_out.size(), &out[0]);
  }
};