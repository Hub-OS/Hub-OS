#include "bnBufferWriter.h"

BufferWriter::BufferWriter()
{
}

void BufferWriter::WriteTerminatedString(Poco::Buffer<char>& buffer, const std::string& text)
{
  buffer.append(text.c_str(), text.size());
  buffer.append(0);
}
