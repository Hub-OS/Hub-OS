#pragma once

template<typename T, size_t WINDOW_LEN>
class RollingWindow {
public:
  size_t Length() {
    return currLen;
  }

  T GetEMA() {
    return ema;
  }

  void SetSmoothing(double value) {
    smoothing = value;
  }

  void Push(T entry) {
    currLen = std::min(currLen + 1, WINDOW_LEN);

    if (currLen == 1) {
      ema = entry;
      return;
    }

    double newEma = (static_cast<double>(entry) * (smoothing / (1.0 + WINDOW_LEN)))
      + static_cast<double>(ema) * (1.0 - (smoothing / (1.0 + WINDOW_LEN)));

    ema = static_cast<T>(newEma);
  }
private:
  double smoothing{ 2 };
  T ema{};
  size_t currLen{};
};
