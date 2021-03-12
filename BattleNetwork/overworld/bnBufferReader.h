#pragma once

#include <Poco/Buffer.h>

namespace Overworld
{
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
      T result;
      size_t size = sizeof(T);
      std::memcpy(&result, buffer.begin()+offset, size);
      offset += size;
      return result;
    }

    std::string ReadString(const Poco::Buffer<char>& buffer);
  };
} // namespace Overworld
