#pragma once
#include <string>
#include <SFML/System/FileInputStream.hpp>

class FileUtil {
public:
  static std::string Read(std::string _path) {
    sf::FileInputStream in;

    if(in.open(_path) && in.getSize() > 0) {
        sf::Int64 size = in.getSize();
        char* buffer = new char[size+1];
        in.read(buffer, size);
        buffer[size] = '\0';

        std::string strbuff(buffer);

        delete[] buffer;

        return strbuff;
    }

    return std::string("");
  }
};