#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <assert.h>
#include "bnLogger.h"

/*
Ammature file utility that reads in a file as a string dump.

Has function that finds a key and attempts to parse the value wrapped in quotes
Much can be improved here
*/
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
      if (keyIndex == -1) {
        Logger::Log(error);
        throw std::runtime_error(error);
      }

      std::string s = line.substr(keyIndex + key.size() + 2);
      return s.substr(0, s.find("\""));
  }
};