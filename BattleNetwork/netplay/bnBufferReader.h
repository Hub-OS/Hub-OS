#pragma once

#include <Poco/Buffer.h>
#include <SFML/Graphics/Color.hpp>
#include "../bnLogger.h"

class BufferReader
{
private:
  size_t offset{};

public:
  BufferReader();

  size_t GetOffset();
  void Skip(size_t n);

  // warning: this method breaks if the endianness of the server and client differ!
  // no lifetimes in this language, so forcing you to pass buffer to be explicit
  template <typename T>
  T Read(const Poco::Buffer<char>& buffer)
  {
    T result = static_cast<T>(0);
    size_t size = sizeof(T);

    if (offset + size > buffer.size()) {
      Logger::Log("BufferReader read past end!");
      offset = buffer.size();
      return result;
    }

    std::memcpy(&result, buffer.begin() + offset, size);
    offset += size;
    return result;
  }

  template <typename Size>
  std::string ReadString(const Poco::Buffer<char>& buffer)
  {
    auto length = Read<Size>(buffer);

    return ReadString(buffer, length);
  }

  std::string ReadString(const Poco::Buffer<char>& buffer, size_t length)
  {
    auto remainingBytes = buffer.size() - offset;

    if (remainingBytes < length) {
      Logger::Log("BufferReader read past end!");
      return "";
    }

    auto result = std::string(buffer.begin() + offset, length);
    offset += length;

    return result;
  }

  std::string ReadTerminatedString(const Poco::Buffer<char>& buffer);

  sf::Color ReadRGBA(const Poco::Buffer<char>& buffer);
};
