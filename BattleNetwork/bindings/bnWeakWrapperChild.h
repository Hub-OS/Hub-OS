#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnWeakWrapper.h"

template<typename Parent, typename Child>
class WeakWrapperChild {
private:
  std::shared_ptr<Parent> sharedPtr;
  std::weak_ptr<Parent> weakPtr;
  Child& child;
public:
  WeakWrapperChild(std::weak_ptr<Parent> weakPtr, Child& child) : weakPtr(weakPtr), child(child) {}

  void OwnParent() {
    sharedPtr = weakPtr.lock();
  }

  std::weak_ptr<Parent> GetParentWeak() {
    return weakPtr;
  }

  Child& Unwrap() {
    if (weakPtr.expired()) {
      throw std::runtime_error("data deleted");
    }

    return child;
  }
};
#endif