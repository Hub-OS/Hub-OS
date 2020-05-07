#pragma once
#include <functional>

template<typename FnSig>
class Callback {
  std::function<FnSig> slot;

public:
  Callback() {
  }

  ~Callback() = default;

  void Slot(decltype(slot) slot) {
    Callback::slot = slot;
  }

  void operator()() { slot? slot() : 0; }
};
