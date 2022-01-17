#pragma once
#include <string>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <assert.h>

#include "bnLogger.h"

#include <SFML/System/InputStream.hpp>

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
    WriteStream(const std::filesystem::path& path) {
      isOK = true;
#ifdef __ANDROID_NDK__
      m_file = std::fopen(path.native().c_str(), "w");
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
      errno_t err = _wfopen_s(&m_file, path.native().c_str(), L"w");
      if (err != 0) {
        isOK = false;

        // todo: strerror_s is not supported in MSVC??
        //char buf[strerror_s(err) + 1];
        //strerror_s(buf, sizeof buf, err);
        //Logger::Logf("cannot open file '%s': %s\n", path.c_str(), buf);
        Logger::Log(LogLevel::critical, "Cannot open file " + path.u8string());
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

  static std::string Read(const std::filesystem::path& _path) {
    std::uintmax_t filesize;
    try {
      filesize = std::filesystem::file_size(_path);
    }
    catch (std::filesystem::filesystem_error& e) {
      Logger::Logf(LogLevel::critical, "Failed to read file \"%s\": %s", _path.u8string().c_str(), e.what());
      return "";
    }
    std::ifstream f(_path, std::ios::in | std::ios::binary);
    std::string buf(filesize, '\0');
    if (!f.read(buf.data(), filesize)) {
      Logger::Logf(LogLevel::critical, "Failed to read file \"%s\": eof = %d, bad = %d, fail = %d", _path.u8string().c_str(), f.eof(), f.bad(), f.fail());
      return "";
    }
    return buf;
  }

  static std::string ValueOf(std::string key, std::string line) {
    int keyIndex = (int)line.find(key);
    std::string error("Key '" + key + "' was not found in line '" + line + "'");
    if (keyIndex == -1) {
      Logger::Log(LogLevel::critical, error);
      throw std::runtime_error(error);
    }

    std::string s = line.substr(keyIndex + key.size() + 2);
    return s.substr(0, s.find("\""));
  }
};

class StdFilesystemInputStream : public sf::InputStream {
private:
  std::filesystem::path path;
  std::ifstream stream;

public:
  explicit StdFilesystemInputStream(const std::filesystem::path& path);
  sf::Int64 read(void* data, sf::Int64 size) override;
  sf::Int64 seek(sf::Int64 position) override;
  sf::Int64 tell() override;
  sf::Int64 getSize() override;
};
