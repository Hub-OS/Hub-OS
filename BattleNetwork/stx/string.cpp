#include "string.h"
#include <sstream>

namespace stx {
  std::string replace(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

  std::vector<std::string> tokenize(const std::string& str, char delim) {
    std::vector<std::string> res;

    std::istringstream f(str);
    std::string s;
    while (std::getline(f, s, delim)) {
      res.push_back(s);
    }

    return  res;
  }

  bool insensitive_equals(const std::string_view& a, const std::string_view& b) {
    if (a.size() != b.size()) {
      return false;
    }

    for (size_t i = 0; i < a.size(); i++) {
      if (toupper(a[i]) != toupper(b[i])) {
        return false;
      }
    }

    return true;
  }

  std::string rand_alphanum(size_t n)
  {
    std::vector<char> letters = {
      'a','b','c','d','e','f','g','h','i','j', 'k','l','m',
      'n','o','p','q', 'r','s','t','u','v','w','x', 'y','z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    std::string str;
    for (size_t i = 0; i < n; i++) {
      str += letters[rand() % letters.size()];
    }

    return str;
  }
  stx::result_t<int> to_int(const std::string& str)
  {
    int result{};

    auto [ptr, ec] { std::from_chars(str.data(), str.data() + str.size(), result) };

    if (ec == std::errc::invalid_argument)
    {
      return error<int>("Input is not an int");
    }
    else if (ec == std::errc::result_out_of_range)
    {
      return error<int>("Input number is larger than an int");
    }

    // if (ec == std::errc())
    return ok(result);
  }
  stx::result_t<float> to_float(const std::string& str)
  {
    float result{};

    auto [ptr, ec] { std::from_chars(str.data(), str.data() + str.size(), result) };

    if (ec == std::errc::invalid_argument)
    {
      return error<float>("Input is not a float");
    }
    else if (ec == std::errc::result_out_of_range)
    {
      return error<float>("Input number is larger than a float");
    }

    // if (ec == std::errc())
    return ok(result);
  }
}