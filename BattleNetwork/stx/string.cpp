#include "string.h"

namespace stx {
  std::string replace(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
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
}