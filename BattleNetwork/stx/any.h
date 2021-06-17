#pragma once
/*
* original implementation: https://codereview.stackexchange.com/questions/219075/implementation-of-stdany
* patched by Mav for ONB project
*
* this is a replacement file for std::any for compilers behind-on-the-times
*
* missing: [any.class]/3 https://timsong-cpp.github.io/cppwp/n4659/any.class#3
*/
#include <memory>
#include <utility>
#include <typeinfo>

namespace stx {

template <typename T>
struct is_in_place_type : std::false_type {};

template <typename T>
struct is_in_place_type<std::in_place_type_t<T>> : std::true_type {};

class any {
  template <typename ValueType>
  friend const ValueType *any_cast(const any *) noexcept;

  template <typename ValueType>
  friend ValueType *any_cast(any *) noexcept;

public:
  // constructors

  constexpr any() noexcept = default;

  any(const any &other) {
    if (other.instance) {
      instance = other.instance->clone();
    }
  }

  any(any &&other) noexcept
    : instance(std::move(other.instance)) {}

  ~any() = default;

  template <typename ValueType, typename = std::enable_if_t<
    !std::is_same_v<std::decay_t<ValueType>, any> &&
    !is_in_place_type<std::decay_t<ValueType>>::value &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>
  >>
  any(ValueType &&value) {
    emplace<std::decay_t<ValueType>>(std::forward<ValueType>(value));
  }

  template <typename ValueType, typename... Args, typename = std::enable_if_t<
    std::is_constructible_v<std::decay_t<ValueType>, Args...> &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>
  >>
  explicit any(std::in_place_type_t<ValueType>, Args &&... args) {
    emplace<std::decay_t<ValueType>>(std::forward<Args>(args)...);
  }

  template <typename ValueType, typename List, typename... Args, typename = std::enable_if_t<
    std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<List> &, Args...> &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>
  >>
  explicit any(std::in_place_type_t<ValueType>, std::initializer_list<List> list, Args &&... args) {
    emplace<std::decay_t<ValueType>>(list, std::forward<Args>(args)...);
  }

  // assignment operators

  any &operator=(const any &rhs) {
    any(rhs).swap(*this);
    return *this;
  }

  any &operator=(any &&rhs) noexcept {
    any(std::move(rhs)).swap(*this);
    return *this;
  }

  template <typename ValueType>
  std::enable_if_t<
    !std::is_same_v<std::decay_t<ValueType>, any> &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>,
    any &
  >
  operator=(ValueType &&rhs) {
    any(std::forward<ValueType>(rhs)).swap(*this);
    return *this;
  }

  // modifiers

  template <typename ValueType, typename... Args>
  std::enable_if_t<
    std::is_constructible_v<std::decay_t<ValueType>, Args...> &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>,
    std::decay_t<ValueType> &
  >
  emplace(Args &&... args) {
    auto new_inst = std::make_unique<storage_impl<std::decay_t<ValueType>>>(std::forward<Args>(args)...);
    std::decay_t<ValueType> &value = new_inst->value;
    instance = std::move(new_inst);
    return value;
  }

  template <typename ValueType, typename List, typename... Args>
  std::enable_if_t<
    std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<List> &, Args...> &&
    std::is_copy_constructible_v<std::decay_t<ValueType>>,
    std::decay_t<ValueType> &
  >
  emplace(std::initializer_list<List> list, Args &&... args) {
    auto new_inst = std::make_unique<storage_impl<std::decay_t<ValueType>>>(list, std::forward<Args>(args)...);
    std::decay_t<ValueType> &value = new_inst->value;
    instance = std::move(new_inst);
    return value;
  }

  void reset() noexcept {
    instance.reset();
  }

  void swap(any &other) noexcept {
    std::swap(instance, other.instance);
  }

  friend void swap(any& lhs, any& rhs)
  {
    lhs.swap(rhs);
  }

  // observers

  bool has_value() const noexcept {
    return static_cast<bool>(instance);
  }

  const std::type_info &type() const noexcept {
    return instance ? instance->type() : typeid(void);
  }

private:
  struct storage_base;

  std::unique_ptr<storage_base> instance;

  struct storage_base {
    virtual ~storage_base() = default;

    virtual const std::type_info &type() const noexcept = 0;
    virtual std::unique_ptr<storage_base> clone() const = 0;
  };

  template <typename ValueType>
  struct storage_impl final : public storage_base {
    template <typename... Args>
    storage_impl(Args &&... args)
      : value(std::forward<Args>(args)...) {}

    const std::type_info &type() const noexcept override {
      return typeid(ValueType);
    }

    std::unique_ptr<storage_base> clone() const override {
      return std::make_unique<storage_impl<ValueType>>(value);
    }

    ValueType value;
  };
};

} // stx

namespace stx {

class bad_any_cast : public std::bad_cast {
public:
  const char *what() const noexcept override {
    return "bad any cast";
  }
};

// C++20
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// any_cast

template <typename ValueType>
ValueType any_cast(const any &anything) {
  using value_type_cvref = remove_cvref_t<ValueType>;
  static_assert(std::is_constructible_v<ValueType, const value_type_cvref &>, "program is ill-formed");
  if (auto *value = any_cast<value_type_cvref>(&anything)) {
    return static_cast<ValueType>(*value);
  } else {
    throw bad_any_cast();
  }
}

template <typename ValueType>
ValueType any_cast(any &anything) {
  using value_type_cvref = remove_cvref_t<ValueType>;
  static_assert(std::is_constructible_v<ValueType, value_type_cvref &>, "program is ill-formed");
  if (auto *value = any_cast<value_type_cvref>(&anything)) {
    return static_cast<ValueType>(*value);
  } else {
    throw bad_any_cast();
  }
}

template <typename ValueType>
ValueType any_cast(any &&anything) {
  using value_type_cvref = remove_cvref_t<ValueType>;
  static_assert(std::is_constructible_v<ValueType, value_type_cvref>, "program is ill-formed");
  if (auto *value = any_cast<value_type_cvref>(&anything)) {
    return static_cast<ValueType>(std::move(*value));
  } else {
    throw bad_any_cast();
  }
}

template <typename ValueType>
const ValueType *any_cast(const any *anything) noexcept {
  if (!anything) return nullptr;
  auto *storage = dynamic_cast<any::storage_impl<ValueType> *>(anything->instance.get());
  if (!storage) return nullptr;
  return &storage->value;
}

template <typename ValueType>
ValueType *any_cast(any *anything) noexcept {
  return const_cast<ValueType *>(any_cast<ValueType>(static_cast<const any *>(anything)));
}

// make_any

template <typename ValueType, typename... Args>
any make_any(Args &&... args) {
  return any(std::in_place_type<ValueType>, std::forward<Args>(args)...);
}

template <typename ValueType, typename List, typename... Args>
any make_any(std::initializer_list<List> list, Args &&... args) {
  return any(std::in_place_type<ValueType>, list, std::forward<Args>(args)...);
}

} // stx
