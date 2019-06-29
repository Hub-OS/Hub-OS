#pragma once
#include <fstream>
#include <map>
#include <vector>
#include "bnChipFolder.h"
#include "bnFileUtil.h"
#include <iostream>
#include "bnLogger.h"

/**
 * @class ChipFolderCollection
 * @author mav
 * @date 05/05/19
 * @brief Provides utilities to create, edit, copy, and read a player's folders
 * 
 * Folders are read from a text file with chip names and codes
 */
class ChipFolderCollection {
private:
  std::map<std::string, ChipFolder*> collection; /*!< All folders by name */

public:

  /**
   * @brief Query if a folder with the given name exists
   * @param name of the folder
   * @return true of found, false otherwise
   */
  bool HasFolder(std::string name) {
    return (collection.find(name) != collection.end());
  }

  /**
   * @brief Creates a folder with the give name
   * @param name of the new folder
   * @return true if created, false if folder already exists
   */
  bool MakeFolder(std::string name) {
    if (this->HasFolder(name)) return false;

    collection[name] = new ChipFolder();

    return true;
  }

  /**
   * @brief Get the folder by name
   * @param name of the folder to get
   * @param folder handle to the folder 
   * @return true if loaded, false if the folder was not found
   */
  bool GetFolder(std::string name, ChipFolder*& folder) {
    if (!this->HasFolder(name)) return false;

    folder = collection[name];

    return true;
  }
 
  /**
   * @brief Get a list of the possible folder names
   * @return std::vector<std::string>
   */
  std::vector<std::string> GetFolderNames() {
    std::vector<std::string> v;
    for (std::map<std::string, ChipFolder*>::iterator it = collection.begin(); it != collection.end(); ++it) {
      v.push_back(it->first);
    }

    return v;
  }

  /**
   * @brief Changes a folder name 
   * @param name new name
   * @param folder folder to change name of
   * @return false if the name already exists or folder is null, true if succeeded
   */
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

  /**
   * @brief Loads all folders and loads chip data if data is valid
   * @param path to folder
   * @return ChipFolderCollection
   * 
   * If a chip is malformed, it will show erroneous info on the menus and won't be used in battle
   */
  static ChipFolderCollection ReadFromFile(const std::string& path) {
    string data = FileUtil::Read(path);

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

    /**
   * @brief Writes all in-game folders to a file and overwrites if a file with the same path exists
   * @param path to folder
   * @return true if written successfully, false otherwise
   */
    bool WriteToFile(const std::string& path) {
      try {
        FileUtil::WriteStream ws(path);

        for (auto curr = this->collection.begin(); curr != this->collection.end(); curr++) {
          ws << "Folder title=\"" << curr->first << "\"" << ws.endl();

          auto writableFolder = curr->second;
          for (auto iter = writableFolder->Begin(); iter != writableFolder->End(); iter++) {
            ws << "   Chip name=\"" << iter->GetShortName() << " code=\"" << iter->GetCode()
               << ws.endl();
          }

          ws << ws.endl(); // space between folders
        }
      }catch(std::exception& e) {
        Logger::Log(std::string("Writing chip folder collection failed: ") + e.what());
        return false;
      }

      return true;
    }
};