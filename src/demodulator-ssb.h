#pragma once

#include<algorithm>
#include "dsp.h"

class SSBDemodulator {
private:
  std::vector<float> coefs;
  Downsampler downsamplerI;
  Downsampler downsamplerQ;
  std::vector<float> coefsHilbert;
  FIRFilter filterDelay;
  FIRFilter filterHilbert;
  std::vector<float> coefsSide;
  FIRFilter filterSide;
  double hilbertMul;
  ExpAverage powerLongAvg;
  ExpAverage powerShortAvg;
  double sigRatio;
  double relSignalPower;

public:
  SSBDemodulator(size_t inRate, size_t outRate, size_t filterFreq, bool upper, size_t kernelLen)
    : coefs(getLowPassFIRCoeffs(inRate, 10000, kernelLen)),
      downsamplerI(inRate, outRate, coefs),
      downsamplerQ(inRate, outRate, coefs),
      coefsHilbert(getHilbertCoeffs(kernelLen, upper)),
      filterDelay((coefsHilbert)),
      filterHilbert(coefsHilbert),
      coefsSide(getLowPassFIRCoeffs(outRate, filterFreq, kernelLen)),
      filterSide(coefsSide),
      hilbertMul(upper ? -1 : 1),
      powerLongAvg(outRate * 5, false),
      powerShortAvg(outRate * 0.5, false),
      sigRatio(double(inRate)/ double(outRate)),
      relSignalPower(0)
  {
  }

  /**
   * Demodulates the given I/Q samples.
   * @param {Float32Array} samplesI The I component of the samples
   *     to demodulate.
   * @param {Float32Array} samplesQ The Q component of the samples
   *     to demodulate.
   * @returns {Float32Array} The demodulated sound.
   */
  std::vector<float> demodulateTuned(const std::vector<float> &samplesI, const std::vector<float> &samplesQ) {
    std::vector<float> I, Q;
    downsamplerI.downsample(samplesI, I);
    downsamplerQ.downsample(samplesQ, Q);

    double specSqrSum = 0;
    double sigSqrSum = 0;
    filterDelay.loadSamples(I);
    filterHilbert.loadSamples(Q);
    std::vector<float> prefilter(I.size());
    for (int i = 0; i < prefilter.size(); ++i) {
      prefilter[i] = filterDelay.getDelayed(i) + filterHilbert.get(i) * hilbertMul;
    }
    filterSide.loadSamples(prefilter);
    std::vector<float> out(I.size());
    for (int i = 0; i < out.size(); ++i) {
      double sig = filterSide.get(i);
      double power = sig * sig;
      sigSqrSum += power;
      double stPower = powerShortAvg.add(power);
      double ltPower = powerLongAvg.add(power);
      double multi = 0.9 * std::max(1.0, sqrt(2.0 / std::min(1.0 / 128.0, std::max(ltPower, stPower))));
      out[i] = multi * filterSide.get(i);
      size_t origIndex = i * sigRatio;
      float origI = samplesI[origIndex];
      float origQ = samplesQ[origIndex];
      specSqrSum += origI * origI + origQ * origQ;
    }

    relSignalPower = sigSqrSum / specSqrSum;
    return std::move(out);
  }

  double getRelSignalPower() {
    return relSignalPower;
  }
};


class Demodulator_SSB: public Demodulator {
private:
  const size_t INTER_RATE = 48000;
  SSBDemodulator demodulator;
  std::vector<float> filterCoefs;
  Downsampler downSampler;

public:
  Demodulator_SSB(size_t inRate, size_t outRate, size_t bandwidth, bool upper)
    : demodulator(inRate, INTER_RATE, bandwidth, upper, 151),
      filterCoefs(getLowPassFIRCoeffs(INTER_RATE, 10000, 41)),
      downSampler(INTER_RATE, outRate, filterCoefs)
  {
  }

  ~Demodulator_SSB() {
  }

  virtual size_t process(const std::vector<float> &samplesI, const std::vector<float> &samplesQ, std::vector<float> &left, std::vector<float> &right) {
    std::vector<float> demodulated = demodulator.demodulateTuned(samplesI, samplesQ);
    downSampler.downsample(demodulated, left);
    right.resize(left.size());
    right.assign(left.begin(), left.end());
    return left.size();
  }

  virtual double getRelSignalLevel() {
    return pow(demodulator.getRelSignalPower(), 0.17);
  }
};