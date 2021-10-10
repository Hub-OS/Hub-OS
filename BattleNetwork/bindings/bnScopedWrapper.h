#pragma once
#ifdef BN_MOD_SUPPORT

#include <memory>

template<typename T>
class ScopedWrapper {
private:
  T& ref;
  std::shared_ptr<bool> valid;
public:
  ScopedWrapper(T& ref) : ref(ref) { valid = std::make_shared<bool>(true); }
  ~ScopedWrapper() { *valid = false; }

  T& Unwrap() {
    if (!*valid) {
      throw std::runtime_error("data deleted");
    }

    return ref;
  }
};
#endif