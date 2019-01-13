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
      assert(keyIndex > -1 && "Key was not found in file.");
      std::string s = line.substr(keyIndex + key.size() + 2);
      return s.substr(0, s.find("\""));
  }
};