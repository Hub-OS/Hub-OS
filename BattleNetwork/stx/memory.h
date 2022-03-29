#pragma once
#include <memory>
#include <functional>

namespace stx {
  template <typename Base>
  class enable_shared_from_base : public std::enable_shared_from_this<Base> {
  public:
    template <typename Derived>
    std::shared_ptr<Derived> shared_from_base()
    {
      return std::static_pointer_cast<Derived>(std::enable_shared_from_this<Base>::shared_from_this());
    }

    template <typename Derived>
    std::weak_ptr<Derived> weak_from_base()
    {
      return std::static_pointer_cast<Derived>(std::enable_shared_from_this<Base>::shared_from_this());
    }
  };

  /*
    defer_t will accept a void functor and it will execute the contents at the deconstructor
  */
  struct defer_t {
    std::function<void()> func;
    defer_t(const std::function<void()>& func) : func(func) {}

    ~defer_t() {
      func();
    }
  };
}

/*
  we want to create simple Go-like syntax so we must used macros to create
  hidden and unused variable names
*/
#define DEFER_VAR_NAME_CONCAT(x,y) x ## y
#define DEFER_VAR_NAME(x,y) DEFER_VAR_NAME_CONCAT(x,y)
#define defer(x) stx::defer_t DEFER_VAR_NAME(_defer,__LINE__)([&]{ x; })
