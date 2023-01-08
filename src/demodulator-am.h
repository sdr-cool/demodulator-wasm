#pragma once

#include <algorithm>
#include "dsp.h"

class AMDemodulator {
private:
  std::vector<float> coefs;
  Downsampler downsamplerI;
  Downsampler downsamplerQ;
  double sigRatio;
  double relSignalPower;

public:
  AMDemodulator(double inRate, double outRate, double filterFreq, size_t kernelLen)
    : coefs(getLowPassFIRCoeffs(inRate, filterFreq, kernelLen)),
      downsamplerI(inRate, outRate, coefs),
      downsamplerQ(inRate, outRate, coefs),
      sigRatio(inRate / outRate),
      relSignalPower(0)
  {
  }

  std::vector<float> demodulateTuned(const std::vector<float>& samplesI, const std::vector<float> &samplesQ) {
    std::vector<float> I, Q;
    downsamplerI.downsample(samplesI, I);
    downsamplerQ.downsample(samplesQ, Q);
    double iAvg = average(I);
    double qAvg = average(Q);
    std::vector<float> out(I.size());

    double specSqrSum = 0;
    double sigSqrSum = 0;
    double sigSum = 0;
    for (int i = 0; i < out.size(); ++i) {
      double iv = I[i] - iAvg;
      double qv = Q[i] - qAvg;
      double power = iv * iv + qv * qv;
      double ampl = sqrt(power);
      out[i] = ampl;
      size_t origIndex = i * sigRatio;
      double origI = samplesI[origIndex];
      double origQ = samplesQ[origIndex];
      specSqrSum += origI * origI + origQ * origQ;
      sigSqrSum += power;
      sigSum += ampl;
    }
    double halfPoint = sigSum / out.size();
    for (int i = 0; i < out.size(); ++i) {
      out[i] = (out[i] - halfPoint) / halfPoint;
    }
    relSignalPower = sigSqrSum / specSqrSum;
    return out;
  }

  double getRelSignalPower() {
    return relSignalPower;
  }
};


class Demodulator_AM: public Demodulator {
private:
  const double INTER_RATE = 48000;
  double filterF;
  AMDemodulator demodulator;
  std::vector<float> filterCoefs;
  Downsampler downSampler;

public:
  Demodulator_AM(double inRate, double outRate, double bandwidth)
    : filterF(bandwidth / 2),
      demodulator(inRate, INTER_RATE, filterF, 351),
      filterCoefs(getLowPassFIRCoeffs(INTER_RATE, 10000, 41)),
      downSampler(INTER_RATE, outRate, filterCoefs)
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