#pragma once
#include <string>
#include <cstring>
#include <sstream>
#include <assert.h>
#include <SFML/System.hpp>

#include "bnLogger.h"

#ifdef __ANDROID_NDK__
#include <android/asset_manager.h>
//#include <SFML/System/Android/Activity.hpp>
#include <SFML/System/Lock.hpp>
#endif

/**
 * @class FileUtil
 * @author mav
 * @date 04/05/19
 * @brief Ammature file utility that reads in a file as a string dump.
 * 
 * Has function that finds a key and attempts to parse the value wrapped in quotes.
 * Much can be improved here as this was originally legacy code.
 */
class FileUtil {
public:
  class WriteStream {
  private:
    std::FILE* m_file; //!< stdio file stream

    bool isOK;

  public:
    WriteStream(const std::string& path) {
      isOK = true;
#ifdef __ANDROID_NDK__
      m_file = std::fopen(path.c_str(), "w");
      if(!m_file) {
        isOK = false;
      }
#else
  #ifndef WIN32
      m_file = std::fopen(path.c_str(), "w");
      if (!m_file) {
        isOK = false;
      }
  #else
      errno_t err = fopen_s(&m_file, path.c_str(), "w");
      if (err != 0) {
        isOK = false;
        
        // todo: strerror_s is not supported in MSVC??
        //char buf[strerror_s(err) + 1];
        //strerror_s(buf, sizeof buf, err);
        //Logger::Logf("cannot open file '%s': %s\n", path.c_str(), buf);
        Logger::Logf("Cannot open file %s", path.c_str());
      }
  #endif
#endif
    }

    ~WriteStream() {
      if(m_file) {
        std::fclose(m_file);
      }
    }

    const char endl() const {
      return '\n';
    }

    WriteStream& operator<<(const char* buffer) {
      std::fwrite(buffer, sizeof(char), std::strlen(buffer), m_file);
      return *this;
    }

    WriteStream& operator<<(char bit) {
      char* ptr = &bit;
      std::fwrite(ptr, sizeof(char), 1, m_file);
      ptr = nullptr;
      return *this;
    }

    WriteStream& operator<<(std::string buffer) {
      std::fwrite(buffer.c_str(), sizeof(char), buffer.size(), m_file);
      return *this;
    }
  };

  static std::string Read(const std::string& _path) {
    sf::FileInputStream in;

    if (in.open(_path) && in.getSize() > 0) {
      sf::Int64 size = in.getSize();
      char* buffer = new char[size];
      in.read(buffer, size);

      std::string strbuff(buffer, size);

      delete[] buffer;

      return strbuff;
    }

    return std::string("");
  }

  static std::string ValueOf(std::string key, std::string line) {
    int keyIndex = (int)line.find(key);
    std::string error("Key '" + key + "' was not found in line '" + line + "'");
    if (keyIndex == -1) {
      Logger::Log(error);
      throw std::runtime_error(error);
    }

    std::string s = line.substr(keyIndex + key.size() + 2);
    return s.substr(0, s.find("\""));
  }
};
