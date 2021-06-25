#pragma once

#include <Poco/Buffer.h>
#include "../bnLogger.h"

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
    Write<Size>(buffer, len);
    buffer.append(text.c_str(), len);
  }

  void WriteTerminatedString(Poco::Buffer<char>& buffer, const std::string& text);
};
