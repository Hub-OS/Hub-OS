#pragma once
#include <functional>

template<typename FnSig>
class Callback {
  std::function<FnSig> slot;

public:
  Callback() {
    slot = [](){};
  }

  ~Callback() = default;

  Callback(const Callback& rhs) {
    slot = rhs.slot;
  }

  void Slot(decltype(slot) slot) {
    this->slot = slot;
  }

  void operator()() { slot(); }
};
