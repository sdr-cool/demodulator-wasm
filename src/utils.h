#pragma once

#include <cstddef>
#include <cmath>
#include "liquid.h"

inline void fill_iq(const char *raw, float *target, size_t len) {
  for (size_t idx = 0; idx < len; idx++) {
    target[idx] = float(raw[idx]) / 128 - 0.995;
  }
}

class DcBlock {
private:
  firfilt_rrrf dc_block;

public:
  DcBlock(unsigned int m, float as) {
    dc_block = firfilt_rrrf_create_dc_blocker(m, as);
  }

  ~DcBlock() {
    firfilt_rrrf_destroy(dc_block);
  }

  void push(const float v) {
    firfilt_rrrf_push(dc_block, v);
  }

  void execute(float *out) {
    firfilt_rrrf_execute(dc_block, out);
  }
};

class AudioResampler {
private:
  const float as = 60.0f;

private:
  double audio_resample_ratio;
  msresamp_rrrf audio_resampler;

public:
  AudioResampler(size_t audio_sample_rate, size_t sample_rate) {
      audio_resample_ratio = double(audio_sample_rate) / double(sample_rate);
      audio_resampler = msresamp_rrrf_create(audio_resample_ratio, as);
  }

  ~AudioResampler() {
    msresamp_rrrf_destroy(audio_resampler);
  }

  size_t get_resample_size(size_t len) {
    return (size_t)ceil((double)(len) * audio_resample_ratio);
  }

  size_t resample(float *demod_out, size_t demod_out_len, float *out) {
    unsigned int written;
    msresamp_rrrf_execute(audio_resampler, demod_out, (int)demod_out_len, out, &written);
    return written;  
  }
};