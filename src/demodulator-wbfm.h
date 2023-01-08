#pragma once

#include "demodulator-nbfm.h"

class Deemphasizer {
public:
  double alpha;
  double val = 0;

public:
  Deemphasizer(double sampleRate, double timeConstant_uS)
    : alpha(1 / (1 + sampleRate * timeConstant_uS / 1e6)), val(0)
  {
  }

  void inPlace(std::vector<float> &samples) {
    for (auto it = samples.begin(); it != samples.end(); ++it) {
      val = val + alpha * (*it - val);
      *it = val;
    }
  }
};

class StereoSeparator {
private:
  const double AVG_COEF = 9999;
  const double STD_THRES = 400;
  std::vector<float> SIN;
  std::vector<float> COS;

  double sin;
  double cos;
  ExpAverage iavg;
  ExpAverage qavg;
  ExpAverage cavg;

public:
  bool found;

public:
  StereoSeparator(double sampleRate, double pilotFreq)
    : found(false), SIN(8001), COS(8001), sin(0), cos(1), iavg(9999, false), qavg(9999, false), cavg(49999, true)
  {
    for (int i = 0; i < 8001; ++i) {
      double freq = (pilotFreq + i / 100 - 40) * 2 * M_PI / sampleRate;
      SIN[i] = ::sin(freq);
      COS[i] = ::cos(freq);
    }
  }

  std::vector<float> separate(const std::vector<float> &samples) {
    std::vector<float> out(samples);
    for (int i = 0; i < out.size(); ++i) {
      double hdev = iavg.add(out[i] * sin);
      double vdev = qavg.add(out[i] * cos);
      out[i] *= sin * cos * 2;
      double corr;
      if (hdev > 0) {
        corr = std::max(-4.0, std::min(4.0, vdev / hdev));
      } else {
        corr = vdev == 0 ? 0 : (vdev > 0 ? 4 : -4);
      }
      double idx = round((corr + 4) * 1000);
      double newSin = sin * COS[idx] + cos * SIN[idx];
      cos = cos * COS[idx] - sin * SIN[idx];
      sin = newSin;
      cavg.add(corr * 10);
    }

    found = cavg.getStd() < STD_THRES;
    return out;
  }
};

class Demodulator_WBFM : public Demodulator {
private:
  const double INTER_RATE = 336000;
  const double MAX_F = 75000;
  const double FILTER = MAX_F * 0.8;
  const double PILOT_FREQ = 19000;
  const double DEEMPH_TC = 50;

  FMDemodulator demodulator;
  std::vector<float> filterCoefs;
  Downsampler monoSampler;
  Downsampler stereoSampler;
  StereoSeparator stereoSeparator;
  Deemphasizer leftDeemph;
  Deemphasizer rightDeemph;

public:
  Demodulator_WBFM(double inRate, double outRate)
    : demodulator(inRate, INTER_RATE, MAX_F, FILTER, 51),
      filterCoefs(getLowPassFIRCoeffs(INTER_RATE, 10000, 41)),
      monoSampler(INTER_RATE, outRate, filterCoefs),
      stereoSampler(INTER_RATE, outRate, filterCoefs),
      stereoSeparator(INTER_RATE, PILOT_FREQ),
      leftDeemph(outRate, DEEMPH_TC),
      rightDeemph(outRate, DEEMPH_TC)
  {
  }

  virtual size_t demodulate(const std::vector<float> &samplesI, const std::vector<float> &samplesQ, std::vector<float> &left, std::vector<float> &right) {
    std::vector<float> demodulated = demodulator.demodulateTuned(samplesI, samplesQ);
    monoSampler.downsample(demodulated, left);
    right = left;

    std::vector<float> stereo = stereoSeparator.separate(demodulated);
    if (stereoSeparator.found) {
      std::vector<float> diffAudio(stereo.size());
      stereoSampler.downsample(stereo, diffAudio);
      for (int i = 0; i < diffAudio.size(); ++i) {
        right[i] -= diffAudio[i];
        left[i] += diffAudio[i];
      }
    }

    leftDeemph.inPlace(left);
    rightDeemph.inPlace(right);
    return left.size();
  }

  virtual double getRelSignalLevel() {
    return demodulator.getRelSignalPower();
  }
};