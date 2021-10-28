#pragma once
#ifdef BN_MOD_SUPPORT

#include <memory>

template<typename T>
class ScopedWrapper {
private:
  T& ref;
  std::shared_ptr<bool> valid;
  bool isFirst{};
public:
  ScopedWrapper(T& ref) : ref(ref) {
    valid = std::make_shared<bool>(true);
    isFirst = true;
  }

  ScopedWrapper(const ScopedWrapper<T>& original) : ref(original.ref) {
    valid = original.valid;
    isFirst = false;
  }

  ~ScopedWrapper() {
    if (isFirst) {
      *valid = false;
    }
  }

  inline T& Unwrap() {
    if (!*valid) {
      throw std::runtime_error("data invalid after scope ends");
    }

    return ref;
  }
};

#endif
