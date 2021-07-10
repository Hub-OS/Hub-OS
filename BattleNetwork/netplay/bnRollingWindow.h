#pragma once

template<typename T>
class RollingWindow {
public:
  static constexpr size_t WINDOW_LEN = 1000ll;

  size_t Length() {
    return currLen;
  }

  T GetEMA() {
    return ema;
  }

  void Push(T entry) {
    currLen = std::min(currLen + 1, WINDOW_LEN);

    double smoothing = 2.0;
    double newEma = (static_cast<double>(entry) * (smoothing / (1.0 + WINDOW_LEN)))
      + static_cast<double>(ema) * (1.0 - (smoothing / (1.0 + WINDOW_LEN)));

    ema = static_cast<T>(newEma);
  }
private:
  T ema{};
  size_t currLen{};
};
