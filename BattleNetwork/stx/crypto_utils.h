#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include "result.h"
#include "../stx/string.h"
#include "../crypto/md5.h"

/* STD LIBRARY extensions */
namespace stx {
  namespace detail {
    static bool md5_matches(char* source, char* compare) {
      return std::strcmp(source, compare) == 0;
    }
  }

  static result_t<std::string> generate_md5_from_file(const std::filesystem::path& path) {
    std::vector<char> fileBuffer, md5Buffer;
    md5Buffer.resize(16);
    size_t len{};

    std::ifstream fs(path, std::ios::binary | std::ios::ate);

    if (!fs.good()) {
      return error<std::string>("Unabled to read file " + path.u8string());
    }

    std::ifstream::pos_type pos = fs.tellg();
    len = pos;
    fileBuffer.resize(len);
    fs.seekg(0, std::ios::beg);
    fs.read(&fileBuffer[0], pos);

    MD5(&md5Buffer[0], &fileBuffer[0], len);

    return ok(stx::as_hex(std::string(md5Buffer.data(), md5Buffer.size()), 0));
  }
}
