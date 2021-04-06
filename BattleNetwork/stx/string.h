#pragma once
#include <string>

namespace stx {
      /**
   * @brief Replaces matching string `from` to `to` in source string `str`
   * @param str source string
   * @param from matching pattern
   * @param to replace with
   * @return modified string
   */
  std::string replace(std::string str, const std::string& from, const std::string& to);
}