#pragma once

#include <memory>

template<typename T>
class WeakWrapper {
private:
  std::shared_ptr<T> ownedPtr; // ownership is dropped as soon as it is passed to C++
  std::weak_ptr<T> weakPtr;
public:
  WeakWrapper(std::shared_ptr<T> sharedPtr) : weakPtr(sharedPtr) {}
  WeakWrapper(std::weak_ptr<T> weakPtr) : weakPtr(weakPtr) {}

  void Own() {
    ownedPtr = weakPtr.lock();
  }

  std::shared_ptr<T> Release() {
    auto temp = weakPtr.lock();
    ownedPtr = nullptr;
    return temp;
  }

  // std::shared_ptr<T> Lock() {
  //   return weakPtr.lock();
  // }

  std::shared_ptr<T> Unwrap() {
    auto ptr = weakPtr.lock();

    if (!ptr) {
      throw std::runtime_error("data deleted");
    }

    return ptr;
  }
};
