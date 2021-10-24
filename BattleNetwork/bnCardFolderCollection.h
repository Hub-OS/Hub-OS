#pragma once
#include <fstream>
#include <map>
#include <vector>
#include "bnCardFolder.h"
#include "bnFileUtil.h"
#include <iostream>
#include "bnLogger.h"

/**
 * @class CardFolderCollection
 * @author mav
 * @date 05/05/19
 * @brief Provides utilities to create, edit, copy, and read a player's folders
 * 
 * Folders are read from a text file with card names and codes
 */
class CardFolderCollection {
private:
  std::map<std::string, CardFolder*> collection; /*!< All folders by name */
  std::vector<std::string> order; /*!< Cheaply use index 0 as the saved default folder */
public:

  /**
   * @brief Query if a folder with the given name exists
   * @param name of the folder
   * @return true of found, false otherwise
   */
  bool HasFolder(const std::string& name) {
    return (collection.find(name) != collection.end());
  }

  /**
 * @brief Find the index of a folder with the given name
 * @param name of the folder
 * @return index of folder (base-0). Otherwise -1 to indicate the folder was not found
 */
  int FindFolder(const std::string& name) {
    for (int i = 0; i < (int)order.size(); i++) {
      if (name == order[i]) {
        return i;
      }
    }

    return -1;
  }

  /**
   * @brief Creates a folder with the give name
   * @param name of the new folder
   * @return true if created, false if folder already exists
   */
  bool MakeFolder(const std::string& name) {
    if (HasFolder(name)) return false;

    collection[name] = new CardFolder();

    order.push_back(name);

    return true;
  }

  /**
 * @brief Deletes a folder with the give name
 * @param name of the folder to delete from collection
 * @return true if deleted, false if folder does not exist
 * @warning THIS IS PERMANENT AND CANNOT UNDO
 */
  bool DeleteFolder(const std::string& name) {
    auto iter = collection.find(name);

    if (iter != collection.end()) {
      auto f = std::find(order.begin(), order.end(), iter->first);
      // remove the old name from the order
      order.erase(f);

      collection.erase(iter);
      return true;
    }

    return false;
  }

  /**
   * @brief Get the folder by name
   * @param name of the folder to get
   * @param folder handle to the folder 
   * @return true if loaded, false if the folder was not found
   */
  bool GetFolder(const std::string& name, CardFolder*& folder) {
    if (!HasFolder(name)) return false;

    folder = collection[name];

    return true;
  }

  /**
 * @brief Get the folder by index
 * @param index of the folder to get
 * @param folder handle to the folder
 * @return true if loaded, false if the folder was not found
 */
  bool GetFolder(int index, CardFolder*& folder) {
    if (index >= order.size() || index < 0) return false;

    if (!HasFolder(order[index])) return false;

    folder = collection[order[index]];

    return true;
  }
 
  /**
   * @brief Get a list of the folder names in order
   * @return std::vector<std::string>
   */
  std::vector<std::string> GetFolderNames() const {
    std::vector<std::string> v;
    for (int i = 0; i < order.size(); i++) {
      auto iter = collection.find(order[i]);
      v.push_back(iter->first);
    }

    return v;
  }

  /**
   * @brief Changes a folder name 
   * @param name new name
   * @param folder folder to change name of
   * @return false if the name already exists or folder is null, true if succeeded
   */
  bool SetFolderName(const std::string& name, CardFolder* folder) {
    if (!folder) return false;

    if (HasFolder(name)) {
      return false;
    }

    for (std::map<std::string, CardFolder*>::iterator it = collection.begin(); it != collection.end(); ++it) {
      if (it->second == folder) {

        // insert the new name where the old name was
        auto f = std::find(order.begin(), order.end(), it->first);
        order.insert(f, name);

        // remove the old name
        order.erase(std::remove(order.begin(), order.end(), it->first));

        // remove the folder
        collection.erase(it);
        break;
      }
    }

    collection[name] = folder;

    return true;
  }

  /**
 * @brief Swaps position with a folder
 * @param from the index of the folder to swap
 * @param with the index of the folder to swap with
 */
  void SwapOrder(int from, int with) {
    if (!(from >= 0 && from < order.size() && with >= 0 && with < order.size())) return;
    std::iter_swap(order.begin()+from, order.begin()+with);
  }
};