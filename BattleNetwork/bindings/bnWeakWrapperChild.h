#pragma once
#include "bnWeakWrapper.h"

#ifdef BN_MOD_SUPPORT

template<typename Parent, typename Child>
class WeakWrapperChild {
private:
  std::shared_ptr<Parent> sharedPtr;
  std::weak_ptr<Parent> weakPtr;
  Child& child;
public:
  WeakWrapperChild(std::weak_ptr<Parent> weakPtr, Child& child) : weakPtr(weakPtr), child(child) {}

  inline void OwnParent() {
    sharedPtr = weakPtr.lock();
  }

  inline std::weak_ptr<Parent> GetParentWeak() {
    return weakPtr;
  }

  inline std::shared_ptr<Parent> UnwrapParent() {
    auto ptr = weakPtr.lock();

    if (!ptr) {
      throw std::runtime_error("data deleted");
    }

    return ptr;
  }

  inline Child& Unwrap() {
    if (weakPtr.expired()) {
      throw std::runtime_error("data deleted");
    }

    return child;
  }
};

#endif