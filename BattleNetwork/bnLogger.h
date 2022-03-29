#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <fstream>
#include <cstdarg>
#include "stx/string.h"
#include "bnCurrentTime.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

using std::string;
using std::to_string;
using std::cerr;
using std::cout;
using std::endl;

namespace LogLevel {
  const uint8_t silent = 0x00;
  const uint8_t info = 0x01;
  const uint8_t debug = 0x02;
  const uint8_t warning = 0x04;
  const uint8_t critical = 0x08;
  const uint8_t all = info | warning | critical | debug;
};

/*! \brief Thread safe logging utility logs directly to a file */
class Logger {
private:
  static std::mutex m;
  static std::queue<std::string> logs; /*!< get the log stream in order */
  static std::ofstream file; /*!< The file to write to */
  static uint8_t logLevel;

  static void Open(std::ofstream& file) {
    if (!file.is_open()) {
      file.open("log.txt", std::ios::app);
      file << "==============================" << endl;
      file << "Build Hash " << ONB_BUILD_HASH << endl;
      file << "StartTime " << CurrentTime::AsString() << endl;
    }
  }

public:

  /**
  * @breif sets the log level filter so that any message that does not match will not be reported
  */
  static void SetLogLevel(uint8_t level) {
    logLevel = level;
  }

  /**
   * @brief Gets the next log and stores it in the input string
   * @param next input string to store result into
   * @return true if there's more text. False if there's no text to INPUTx.
   */
  static const bool GetNextLog(std::string &next) {
    std::scoped_lock<std::mutex> lock(m);

    if (logs.size() == 0)
        return false;

    next = logs.front();
    logs.pop();

    return (logs.size()+1 > 0);
  }

  /**
   * @brief If first time opening, timestamps file and pushes message to file
   * @param message
   */
  static void Log(uint8_t level, string _message) {
    std::scoped_lock<std::mutex> lock(m);

    if (_message.empty())
      return;

    _message = ErrorLevel(level) + _message;

    Open(file);

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "open mmbn engine", "%s", _message.c_str());
#else
    if ((level & Logger::logLevel) == level) {
      cerr << _message << endl;
    }
#endif

    logs.push(_message);
    file << _message << endl;
  }

  template<typename... Args>
  static void Logf(uint8_t level, const char* fmt, Args&&... args) {
    Log(level, stx::format(512, fmt, args...));
  }

  // Print and log
  static void PrintLog(uint8_t level, string _message) {
    Log(level, _message);
    cout << _message << endl;
  }

  // Print and log
  template<typename... Args>
  static void PrintLogf(uint8_t level, const char* fmt, Args&&... args) {
    std::string ret = stx::format(512, fmt, args...);
    Log(level, ret);
    cout << ret << endl;
  }

private:
  Logger() { ; }

  /**
   * @brief Dumps queue and closes file
   */
  ~Logger() { file.close(); while (!logs.empty()) { logs.pop(); } }

  static std::string ErrorLevel(uint8_t level) {
    if (level == LogLevel::critical) {
      return "[CRITICAL] ";
    }

    if (level == LogLevel::warning) {
      return "[WARNING] ";
    }

    if (level == LogLevel::debug) {
      return "[DEBUG] ";
    }

    // anything else
    return "[INFO] ";
  }
};
