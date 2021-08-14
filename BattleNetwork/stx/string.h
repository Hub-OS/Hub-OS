#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace stx {
  /**
   * @brief Replaces matching string `from` to `to` in source string `str`
   * @param str source string
   * @param from matching pattern
   * @param to replace with
   * @return modified string
   */
  std::string replace(std::string str, const std::string& from, const std::string& to);

  /**
   * @brief Compares two strings ignoring capitalization
   * @param a
   * @param b
   * @return
   */
  bool insensitive_equals(const std::string_view& a, const std::string_view& b);

  /**
  * @brief Generates a random alpha numeric string of length `n`
  */
  std::string rand_alphanum(size_t n);
}