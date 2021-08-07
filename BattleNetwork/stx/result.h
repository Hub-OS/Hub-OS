#pragma once
#include <string>

/* STD LIBRARY extensions */
namespace stx {
    template<typename T>
    class result_t;

    template<typename T>
    result_t<T> ok(const T& value) {
      return result_t<T>(value);
    }

    template<typename T>
    result_t<T> error(const std::string& reason) {
      return result_t<T>(std::nullptr_t{}, reason);
    }

    template<typename T>
    class result_t {
        T m_value;
        std::string m_error;

        bool m_is_error{};

        public:
        result_t(const std::nullptr_t&, const std::string& reason) : 
          m_is_error(true), 
          m_value(T{}),
          m_error(reason) 
        {

        }

        result_t(const T& value) : m_value(value) 
        {
        }

        bool is_error() {
            return this->m_is_error;
        }

        T value() {
            return this->m_value;
        }

        const char* error_cstr() {
            return this->m_error.c_str();
        }
    };
}