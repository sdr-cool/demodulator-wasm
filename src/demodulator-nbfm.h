#pragma once

#include <algorithm>
#include "dsp.h"

class FMDemodulator {
private:
  double AMPL_CONV;

  std::vector<float> coefs;
  Downsampler downsamplerI;
  Downsampler downsamplerQ;
  double lI;
  double lQ;
  double relSignalPower;

public:
  FMDemodulator(double inRate, double outRate, double maxF, double filterFreq, size_t kernelLen)
    : AMPL_CONV(outRate / (2 * M_PI * maxF)),
      coefs(getLowPassFIRCoeffs(inRate, filterFreq, kernelLen)),
      downsamplerI(inRate, outRate, coefs),
      downsamplerQ(inRate, outRate, coefs),
      lI(0), lQ(0), relSignalPower(0)
  {
  }

  std::vector<float> demodulateTuned(const std::vector<float>& samplesI, const std::vector<float> &samplesQ) {
    std::vector<float> I, Q;
    downsamplerI.downsample(samplesI, I);
    downsamplerQ.downsample(samplesQ, Q);
    std::vector<float> out(I.size());

    double prev = 0;
    double difSqrSum = 0;
    for (int i = 0; i < out.size(); ++i) {
      double real = lI * I[i] + lQ * Q[i];
      double imag = lI * Q[i] - I[i] * lQ;
      double sgn = 1;
      double circ = 0;
      double ang = 0;
      double div = 1;
      if (real < 0) {
        sgn = -sgn;
        real = -real;
        circ = M_PI;
      }
      if (imag < 0) {
        sgn = -sgn;
        imag = -imag;
        circ = -circ;
      }
      if (real > imag) {
        div = imag / real;
      } else if (real != imag) {
        ang = -M_PI / 2;
        div = real / imag;
        sgn = -sgn;
      }
      out[i] = circ + sgn *
        (ang + div
               / (0.98419158358617365
                  + div * (0.093485702629671305
                           + div * 0.19556307900617517))) * AMPL_CONV;
      lI = I[i];
      lQ = Q[i];
      double dif = prev - out[i];
      difSqrSum += dif * dif;
      prev = out[i];
    }

    relSignalPower = 1 - sqrt(difSqrSum / out.size());
    return out;
  }

  double getRelSignalPower() {
    return relSignalPower;
  }
};

class Demodulator_NBFM: public Demodulator {
private:
  double multiple;
  double interRate;
  double filterF;

  FMDemodulator demodulator;
  std::vector<float> filterCoefs;
  Downsampler downSampler;

public:
  Demodulator_NBFM(double inRate, double outRate, double maxF)
    : multiple(1 + floor((maxF - 1) * 7 / 75000)),
      interRate(48000 * multiple),
      filterF(maxF * 0.8),
      demodulator(inRate, interRate, maxF, filterF, 50 * 7 / multiple),
      filterCoefs(getLowPassFIRCoeffs(interRate, 8000, 41)),
      downSampler(interRate, outRate, filterCoefs)
  {
  }

  virtual size_t process(const std::vector<float> &samplesI, const std::vector<float> &samplesQ, std::vector<float> &left, std::vector<float> &right) {
    std::vector<float> demodulated = demodulator.demodulateTuned(samplesI, samplesQ);
    downSampler.downsample(demodulated, left);
    right = left;
    return left.size();
  }

  virtual double getRelSignalLevel() {
    return pow(demodulator.getRelSignalPower(), 0.17);
  }
};
