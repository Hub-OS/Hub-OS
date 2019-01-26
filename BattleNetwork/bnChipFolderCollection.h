#pragma once
#include <fstream>
#include <map>
#include <vector>
#include "bnChipFolder.h"
#include "bnFileUtil.h"
#include <iostream>
#include "bnLogger.h"

class ChipFolderCollection {
private:
  std::map<std::string, ChipFolder*> collection;

public:
  bool HasFolder(std::string name) {
    return (collection.find(name) != collection.end());
  }

  bool MakeFolder(std::string name) {
    if (this->HasFolder(name)) return false;

    collection[name] = new ChipFolder();

    return true;
  }

  bool GetFolder(std::string name, ChipFolder*& folder) {
    if (!this->HasFolder(name)) return false;

    folder = collection[name];

    return true;
  }

  std::vector<std::string> GetFolderNames() {
    std::vector<std::string> v;
    for (std::map<std::string, ChipFolder*>::iterator it = collection.begin(); it != collection.end(); ++it) {
      v.push_back(it->first);
    }

    return v;
  }

  bool SetFolderName(std::string name, ChipFolder* folder) {
    if (!folder) return false;

    if (this->HasFolder(name)) {
      return false;
    }

    for (std::map<std::string, ChipFolder*>::iterator it = collection.begin(); it != collection.end(); ++it) {
      if (it->second == folder) {
        collection[name] = it->second;
        collection.erase(it);
        break;
      }
    }

    return true;
  }

  static ChipFolderCollection ReadFromFile(std::string path) {
    // TODO: put this utility in an input stream class and inhert from that
    string data = FileUtil::Read("resources/database/folders.txt");

    int endline = 0;

    ChipFolderCollection collection;
    ChipFolder* currFolder = nullptr;

    do {
      endline = (int)data.find("\n");
      string line = data.substr(0, endline);

      while (line.compare(0, 1, " ") == 0)
        line.erase(line.begin()); // remove leading whitespaces
      while (line.size() > 0 && line.compare(line.size() - 1, 1, " ") == 0)
        line.erase(line.end() - 1); // remove trailing whitespaces

      if (line.find("Folder") != string::npos) {
        string title = FileUtil::ValueOf("title", line);
        std::cout << "Looking for folder " << title << std::endl;

        if (collection.HasFolder(title)) {
          if (!collection.GetFolder(title, currFolder)) {
            Logger::Log("Failed to get folder " + title);
          }
        }
        else {
          // 10 chars fit on the box 
          title = title.substr(0, 10);

          std::cout << "making folder " << title << std::endl;
          std::cout << "folder status: " << (int)(collection.MakeFolder(title)) << std::endl;;
          std::cout << "retrieved folder: " << (int)collection.GetFolder(title, currFolder) << std::endl;
        }

        std::cout << "folder addr " << currFolder << std::endl;
      }

      if (line.find("Chip") != string::npos) {
        string name = FileUtil::ValueOf("name", line);
        string code = FileUtil::ValueOf("code", line);

        if(currFolder != nullptr) {
          // Query the library for this chip data and push into the folder.
          currFolder->AddChip(CHIPLIB.GetChipEntry(name, code[0]));
        }
        else {
          Logger::Log("Failed to add chip (" + name + ", " + code[0] + "), no folder in build scope!");
        }
      }

      data = data.substr(endline + 1);
    } while (endline > -1);

    return collection;
  }
};