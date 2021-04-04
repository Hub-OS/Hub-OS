#pragma once
#include <functional>

template<typename FnSig>
class Callback {
  std::function<FnSig> slot;
  bool use = false;
public:
  Callback() {
  }

  Callback(const decltype(slot)& slot) : slot(slot), use(true) 
  {}

  ~Callback() = default;

  void Slot(decltype(slot) slot) {
    this->slot = slot;
    use = true;
  }

  void Reset() {
    use = false;
  }

  template<typename... Args>
  void operator()(Args&&... args) { if (!use) return;  slot ? slot(std::forward<decltype(args)>(args)...) : (void(0)); }
};
