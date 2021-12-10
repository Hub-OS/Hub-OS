#pragma once

#include "../bnLogger.h"
#include <Poco/Buffer.h>
#include <limits>

class BufferWriter
{
public:
  BufferWriter();

  // warning: this method breaks if the endianness of the server and client differ!
  // no lifetimes in this language, so forcing you to pass buffer to be explicit
  template <typename T>
  void Write(Poco::Buffer<char>& buffer, const T& data)
  {
    buffer.append((char*)&data, sizeof(T));
  }

  template <typename T>
  void WriteBytes(Poco::Buffer<char>& buffer, T* data, size_t len) {
    buffer.append((char*)data, len);
  }

  template <typename Size>
  void WriteString(Poco::Buffer<char>& buffer, const std::string& text)
  {
    auto len = (Size)text.size();

    if(text.size() > std::numeric_limits<Size>::max()) {
      // prevent corruption by capping len at sizeof(Size)
      len = std::numeric_limits<Size>::max();
      Logger::Log(LogLevel::debug, "BufferWriter::WriteString() String length longer than Size, truncating...");
    }

    Write<Size>(buffer, len);
    buffer.append(text.c_str(), len);
  }

  void WriteTerminatedString(Poco::Buffer<char>& buffer, const std::string& text);
};
