#pragma once

#include <string>
#include "bnChipFolderCollection.h"

using std::string;

/**
 * @class UserSession
 * @author mav
 * @date 07/8/19
 * @brief Describe user play session to save and load
 *
 * Information like time played, their equiped folder, controller config
 * Should also know about all their folders and library when passing around to other
 * scenes.
 *
 * TODO: Should this be made a singleton instead of the chip lib? 
 *       Would be able to access player data anywhere anytime.
 */
class UserSession {
  unsigned long playTime; /*!< How long this user has played */
  std::string equipedFolderName; /*!< API fetches folders by name */
  unsigned selectedNaviIndex; /*!< Which navi is selected. TODO: mods need a uuid system based on hashes */
  ChipFolderCollection folders; /*!< Reference to folder data */
  // ControllerConfig controllerConfig; 
public:
  static UserSession* LoadUserSession(std::string path);
  static void WriteUserSession(std::string path);
  static void CloseUserSession(UserSession* session);
  
};