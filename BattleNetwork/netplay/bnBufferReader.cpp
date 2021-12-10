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

  Logger::Log(LogLevel::critical, "BufferReader read past end!");
  offset = buffer.size();

  return "";
}

sf::Color BufferReader::ReadRGBA(const Poco::Buffer<char>& buffer) {
  auto colorBytes = Read<uint32_t>(buffer);

  return sf::Color(
    colorBytes & 255,
    (colorBytes >> 8) & 255,
    (colorBytes >> 16) & 255,
    (colorBytes >> 24) & 255
  );
}