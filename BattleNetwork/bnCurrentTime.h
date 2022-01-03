#pragma once
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <chrono>

struct CurrentTime {
private:
  // Cross-platform safe timestamp solution from https://stackoverflow.com/a/38034148/3624674
  static inline std::tm localtime_xp(std::time_t timer) {
    std::tm bt{};
  #if defined(__unix__)
    localtime_r(&timer, &bt);
  #elif defined(_MSC_VER)
    localtime_s(&bt, &timer);
  #else
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    bt = *std::localtime(&timer);
  #endif
    return bt;
  }

  // default = "YYYY-MM-DD HH:MM:SS"
  static inline std::string time_stamp(const std::string& fmt = "%F %T") {
    auto bt = localtime_xp(std::time(0));
    char buf[64];
    return { buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &bt) };
  }

public:
  static std::string AsString() {
    return time_stamp("%y-%m-%d %OH:%OM:%OS");
  }

  static std::string AsFormattedString(const std::string& fmt) {
    return time_stamp(fmt.c_str());
  }

  static long long AsMilli() {
    using namespace std::chrono;

    return std::chrono::duration_cast<std::chrono::milliseconds>
      (std::chrono::system_clock::now().time_since_epoch()).count();
  }
};