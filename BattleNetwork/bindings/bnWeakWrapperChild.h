#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnWeakWrapper.h"

template<typename Parent, typename Child>
class WeakWrapperChild {
private:
  std::weak_ptr<Parent> weakPtr;
  Child& child;
public:
  WeakWrapperChild(std::weak_ptr<Parent> weakPtr, Child& child) : weakPtr(weakPtr), child(child) {}

  std::weak_ptr<Parent> GetParentWeak() {
    return weakPtr;
  }

  Child& Unwrap() {
    if (!weakPtr.lock()) {
      throw std::runtime_error("data deleted");
    }

    return child;
  }
};
#endif