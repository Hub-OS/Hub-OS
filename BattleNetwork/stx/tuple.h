#pragma once 

#include <tuple>

/* STD LIBRARY extensions */
namespace stx {
    namespace detail {
        template <class T, class Tuple, std::size_t... I>
        constexpr T* make_from_tuple_impl(Tuple&& t, std::index_sequence<I...>)
        {
            return new T(std::get<I>(std::forward<Tuple>(t))...);
        }
    }

    template <class T, class Tuple>
    constexpr T* make_ptr_from_tuple(Tuple&& t)
    {
    return detail::make_from_tuple_impl<T>(std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }
}