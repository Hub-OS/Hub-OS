#pragma once
#include <charconv>
#include <vector>
#include <string>
#include <string_view>
#include "../stx/result.h"

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
  * @brief splits a string by the delimeter and returns a list of split lines (tokens)
  */
  std::vector<std::string> tokenize(const std::string& str, char delim);

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

  /**
  * @brief Converts input string to an int or returns an error message
  */
  stx::result_t<int> to_int(const std::string& str);

  /**
  * @brief Converts input string to a float or returns an error message
  */
  stx::result_t<float> to_float(const std::string& str);

  /**
  * @brief Trims and formats an input string by restricting the text into a max_cols x max_rows box
  * @warning assumes font used to display is monospaced
  */
  std::string format_to_fit(const std::string& str, size_t max_cols, size_t max_rows);
}