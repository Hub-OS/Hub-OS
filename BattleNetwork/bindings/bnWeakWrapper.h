#pragma once
#ifdef BN_MOD_SUPPORT

#include <memory>

template<typename T>
class WeakWrapper {
private:
  std::shared_ptr<T> ownedPtr; // ownership is dropped as soon as it is passed to C++
  std::weak_ptr<T> weakPtr;
public:
  WeakWrapper(std::shared_ptr<T> sharedPtr) : weakPtr(sharedPtr) {}
  WeakWrapper(std::weak_ptr<T> weakPtr) : weakPtr(weakPtr) {}
  WeakWrapper() {}

  inline void Own() {
    ownedPtr = weakPtr.lock();
  }

  inline std::weak_ptr<T> GetWeak() {
    return weakPtr;
  }

  inline std::shared_ptr<T> Release() {
    auto temp = weakPtr.lock();
    ownedPtr = nullptr;
    return temp;
  }

  inline std::shared_ptr<T> Lock() {
    return weakPtr.lock();
  }

  inline std::shared_ptr<T> Unwrap() {
    auto ptr = weakPtr.lock();

    if (!ptr) {
      throw std::runtime_error("data deleted");
    }

    return ptr;
  }
};
#endif