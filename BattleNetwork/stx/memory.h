#pragma once
#include <memory>

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
}
