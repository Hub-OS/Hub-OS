#pragma once
#include <iostream>
#include <string>
#include <cstdarg>
#include <queue>
#include <mutex>
#include <fstream>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

using std::string;
using std::to_string;
using std::cerr;
using std::endl;

/*! \brief Thread safe logging utility logs directly to a file */
class Logger {
private:
  static std::mutex m;
  static std::queue<std::string> logs; /*!< get the log stream in order */
  static std::ofstream file; /*!< The file to write to */
public:
  static std::mutex* GetMutex() {
    return &m;
  }

  /**
   * @brief Gets the next log and stores it in the input string
   * @param next input string to store result into
   * @return true if there's more text. False if there's no text to input.
   */
  static const bool GetNextLog(std::string &next) {
    if (logs.size() == 0)
      return false;

    next = logs.front();
    logs.pop();

    return (logs.size()+1 > 0);
  }

  /**
   * @brief If first time opening, timestamps file and pushes message to file
   * @param _message
   */
  static void Log(string _message) {
    if (_message.empty())
      return;

    if (!file.is_open()) {
      file.open("log.txt");
      file << "StartTime " << time(0) << endl;
    }

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO,"open mmbn engine","%s",_message.c_str());
#else
    cerr << _message << endl;
#endif

    logs.push(_message);
    file << _message << endl;
  }
  
  /**
   * @brief Uses varadic args to print any string format
   * @param fmt string format
   * @param ... input to match the format 
   */
  static void Logf(const char* fmt, ...) {
    int size = 512;
    char* buffer = 0;
    buffer = new char[size];
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if (size <= nsize) {
      delete[] buffer;
      buffer = 0;
      buffer = new char[nsize + 1];
      nsize = vsnprintf(buffer, size, fmt, vl);
    }
    std::string ret(buffer);
    va_end(vl);
    delete[] buffer;
    cerr << ret << endl;
    logs.push(ret);

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO,"open mmbn engine","%s",ret.c_str());
#else
    if (!file.is_open()) {
      file.open("log.txt");
      file << "StartTime " << time(0) << endl;
    }

    file << ret << endl;
#endif
  }

  static string ToString(float _number) {
    return to_string(_number);
  }

private:
  Logger() { ; }
  
  /**
   * @brief Dumps queue and closes file
   */
  ~Logger() { file.close(); while (!logs.empty()) { logs.pop(); }  }
};