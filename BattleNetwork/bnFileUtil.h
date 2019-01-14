#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <assert.h>

class FileUtil {
public:
  static std::string Read(std::string _path) {
    std::ifstream in(_path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string contents(buffer.str());
    return contents;
  }

  static std::string ValueOf(std::string key, std::string line) {
      int keyIndex = (int)line.find(key);
      std::string error("Key '" + key + "' was not found in line '" + line + "'");
      assert(keyIndex > -1 && error.c_str());
      std::string s = line.substr(keyIndex + key.size() + 2);
      return s.substr(0, s.find("\""));
  }
};