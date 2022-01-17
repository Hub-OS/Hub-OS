#include "bnFileUtil.h"

StdFilesystemInputStream::StdFilesystemInputStream(const std::filesystem::path& path) :
  path(path),
  stream(path, std::ios::in | std::ios::binary) {
}

sf::Int64 StdFilesystemInputStream::read(void* data, sf::Int64 size) {
  if (!stream) {
    return -1;
  }
  stream.read(static_cast<char*>(data), size);
  std::streampos n = stream.gcount();
  stream.clear();
  return n;
}

sf::Int64 StdFilesystemInputStream::seek(sf::Int64 position) {
  if (!stream) {
    return -1;
  }
  stream.seekg(position);
  return stream.tellg();
}

sf::Int64 StdFilesystemInputStream::tell() {
  if (!stream) {
    return -1;
  }
  return stream.tellg();
}

sf::Int64 StdFilesystemInputStream::getSize() {
  if (!stream) {
    return -1;
  }
  return std::filesystem::file_size(path);
}
