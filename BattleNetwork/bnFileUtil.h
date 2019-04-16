#pragma once
#include <string>
#include <SFML/System/FileInputStream.hpp>
#include "bnLogger.h"

/*
Ammature file utility that reads in a file as a string dump.

Has function that finds a key and attempts to parse the value wrapped in quotes
Much can be improved here
*/
class FileUtil {
public:
  static std::string Read(std::string _path) {
    sf::FileInputStream in;

    if (in.open(_path) && in.getSize() > 0) {
      sf::Int64 size = in.getSize();
      char* buffer = new char[size + 1];
      in.read(buffer, size);
      buffer[size] = '\0';

      std::string strbuff(buffer);

      delete[] buffer;

      return strbuff;
    }

    return std::string("");
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