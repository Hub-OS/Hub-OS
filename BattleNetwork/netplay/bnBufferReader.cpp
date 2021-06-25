#include "bnBufferReader.h"

BufferReader::BufferReader()
{
  offset = 0;
}

size_t BufferReader::GetOffset()
{
  return offset;
}

void BufferReader::Skip(size_t n)
{
  offset += n;
}

std::string BufferReader::ReadTerminatedString(const Poco::Buffer<char>& buffer)
{
  auto iter = buffer.begin();

  for (auto i = offset; i < buffer.size(); i++)
  {
    if (*(iter + i) == '\0')
    {
      auto length = i - offset;
      auto result = std::string(iter + offset, length);

      // + 1 for the null terminator
      offset += length + 1;

      return result;
    }
  }

  return "";
}

