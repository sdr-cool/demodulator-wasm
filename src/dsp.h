#pragma once

#include <cstddef>
#include <cmath>
#include <cstdint>
#include <vector>

#include <iostream>
template<class ITER> void debug_view(ITER begin, size_t len = 8) {
  auto end = begin + len;
  while (begin != end)
  {
    std::cout << *begin++ << " ";
  }
  std::cout << std::endl;
}

class Demodulator {
public:
  virtual ~Demodulator() {}
  virtual size_t process(const std::vector<float> &I, const std::vector<float> &Q, std::vector<float> &left, std::vector<float> &right) = 0;
  virtual double getRelSignalLevel() = 0;
};

inline void iqSamplesFromUint8(const std::vector<uint8_t>& buffer, std::vector<float> &outI, std::vector<float> &outQ) {
  // var arr = new Uint8Array(buffer);
  size_t len = buffer.size() / 2;
  outI.resize(len);
  outQ.resize(len);

  for (int i = 0; i < len; ++i) {
    outI[i] = float(buffer[2 * i]) / 128 - 0.995;
    outQ[i] = float(buffer[2 * i + 1]) / 128 - 0.995;
  }
}

inline void shiftFrequency(std::vector<float> *IQ, size_t freq, size_t sampleRate, double &cosine, double &sine) {
  double deltaCos = cos(freq * 2.0 * M_PI / sampleRate);
  double deltaSin = sin(freq * 2.0 * M_PI / sampleRate);
  std::vector<float> &I = IQ[0];
  std::vector<float> &Q = IQ[1];
  // var oI = new Float32Array(I.length);
  // var oQ = new Float32Array(Q.length);
  for (int i = 0; i < I.size(); ++i) {
    float ii = I[i];
    float qi = Q[i];
    I[i] = ii * cosine - qi * sine;
    Q[i] = ii * sine + qi * cosine;
    double newSine = cosine * deltaSin + sine * deltaCos;
    cosine = cosine * deltaCos - sine * deltaSin;
    sine = newSine;
  }
}

class FIRFilter {
private:
  std::vector<float> coefs;
  size_t offset;
  size_t center;
  std::vector<float> curSamples;

public:
  FIRFilter(const std::vector<float>& coefficients):
    coefs(coefficients), offset(coefs.size() - 1), center(coefs.size() / 2), curSamples(offset)
  {
  }

  void loadSamples(const std::vector<float>& samples) {
    size_t sub_start = curSamples.size() - offset;
    size_t sub_end = curSamples.size();
    curSamples.resize(samples.size() + offset);
    std::copy(curSamples.begin() + sub_start, curSamples.begin() + sub_end, curSamples.begin());
    std::copy(samples.begin(), samples.end(), curSamples.begin() + offset);
  }

  double get(size_t index) {
    double out = 0;
    for (int i = 0; i < coefs.size(); ++i) {
      out += coefs[i] * curSamples[index + i];
    }
    return out;
  }

  float getDelayed(size_t index) {
    return curSamples[index + center];
  }
};


class Downsampler {
private:
  FIRFilter filter;// = new FIRFilter(coefficients);
  double rateMul; // = inRate / outRate;

public:
  Downsampler(size_t inRate, size_t outRate, const std::vector<float>& coefficients):
    filter(coefficients), rateMul(double(inRate) / double(outRate))
  {
  }

  void downsample(const std::vector<float>& samples, std::vector<float> &outArr) {
    // debug_view(samples.begin());

    filter.loadSamples(samples);
    outArr.resize(samples.size() / rateMul);
    double readFrom = 0.0;
    for (int i = 0; i < outArr.size(); ++i, readFrom += rateMul) {
      outArr[i] = filter.get(size_t(readFrom));
    }
    // debug_view(outArr.begin());
  }
};

std::vector<float> getLowPassFIRCoeffs(size_t sampleRate, size_t halfAmplFreq, size_t length) {
  length += (length + 1) % 2;
  double freq = double(halfAmplFreq) / double(sampleRate);
  std::vector<float> coefs(length);
  size_t center = length / 2;
  double sum = 0;
  for (int i = 0; i < length; ++i) {
    double val = 0;
    if (i == center) {
      val = 2 * M_PI * freq;
    } else {
      val = sin(2 * M_PI * freq * (i - center)) / (i - center);
      val *= 0.54 - 0.46 * cos(2 * M_PI * i / (length - 1));
    }
    sum += val;
    coefs[i] = val;
  }
  for (int i = 0; i < length; ++i) {
    coefs[i] /= sum;
  }
  return std::move(coefs);
}

std::vector<float> getHilbertCoeffs(size_t length, bool upper) {
  length += (length + 1) % 2;
  size_t center = length / 2;
  std::vector<float> out(length);
  for (int i = 0; i < out.size(); ++i) {
    if ((i % 2) == 0) {
      out[i] = 2 / (M_PI * (center - i));
    }
  }
  return std::move(out);
}

class ExpAverage {
  double avg;
  double std;
  double weight;
  bool opt_std;

public:
  ExpAverage(double _weight, bool _opt_std)
    : avg(0), std(0), weight(_weight), opt_std(_opt_std)
  {
  }

  double add(double value) {
    avg = (weight * avg + value) / (weight + 1);
    if (opt_std) {
      std = (weight * std + (value - avg) * (value - avg)) / (weight + 1);
    }
    return avg;
  }

  double getStd() {
    return std;
  }
};

double average(const std::vector<float> &arr) {
  double sum = 0;
  for (auto it = arr.begin(); it != arr.end(); ++it) sum += *it;
  return sum / arr.size();
}