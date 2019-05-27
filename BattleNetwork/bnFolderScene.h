#pragma once
#include <Swoosh/Ease.h>
#include <Swoosh/Activity.h>

#include "bnCamera.h"
#include "bnChipFolderCollection.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"

<<<<<<< HEAD
class FolderScene : public swoosh::Activity {
private:
  Camera camera;
  ChipFolderCollection& collection;
  ChipFolder* folder;
  std::vector<std::string> folderNames;

  // Menu name font
  sf::Font* font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;
=======
/**
 * @class FolderScene
 * @author mav
 * @date 04/05/19
 * @brief Display folders at top and contents in the center. Player may add, edit, or remove folders.
 * 
 * Use input keys LEFT/RIGHT to navigate through infinite list of folders
 * Use input keys UP/DOWN to scroll through chips inside the folders
 * Use input key A to prompt management menu. User may edit name, remove, edit, or copy.
 * Use input key B to cancel menus and go back to the previous screen
 * 
 * If editting, keyboard input is captured and replaces text immediately
 * If removing, the user is prompted to confirm
 * If copying, the user is prompted to confirm
 * If editting, the user is taken to the FolderEditScene
 */
class FolderScene : public swoosh::Activity {
private:
  Camera camera;
  ChipFolderCollection& collection; /*!< The entire user collection */
  ChipFolder* folder; /*!< Handle to current folder to preview */
  std::vector<std::string> folderNames; /*!< List of all folder names at start */

  // Menu name font
  sf::Font* font; /*!< Font of the  menu name label*/
  sf::Text* menuLabel; /*!< "Folder" text on top-left */

  // Selection input delays
  double maxSelectInputCooldown; /*!< Set to half of a second */
  double selectInputCooldown; /*!< The delay between reading user input */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  // Chip UI font
  sf::Font *chipFont;
  sf::Text *chipLabel;

  sf::Font *numberFont;
  sf::Text *numberLabel;

<<<<<<< HEAD
  // folder menu graphic
=======
  // folder menu graphics
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  sf::Sprite bg;
  sf::Sprite folderBox;
  sf::Sprite folderCursor;
  sf::Sprite scrollbar;
  sf::Sprite folderOptions;
  sf::Sprite element;
  sf::Sprite cursor;
  sf::Sprite folderEquip;
  sf::Sprite chipIcon;

<<<<<<< HEAD
  Animation equipAnimation;
  Animation folderCursorAnimation;
=======
  Animation equipAnimation; /*!< Flashes */
  Animation folderCursorAnimation; /*!< Flashes */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  int currFolderIndex;
  int selectedFolderIndex;

<<<<<<< HEAD
  int maxChipsOnScreen;
  int currChipIndex;
  int numOfChips;
=======
  int maxChipsOnScreen; /*!< The number of chips to display max in box area */
  int currChipIndex; /*!< Current index in current folder */
  int numOfChips; /*!< Number of chips in current folder */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  double totalTimeElapsed;
  double frameElapsed;

  double folderOffsetX;

<<<<<<< HEAD
  bool gotoNextScene;

  int optionIndex;
  bool promptOptions;
  bool enterText;

public:
  virtual void onStart();
=======
  bool gotoNextScene; /*!< If true, user cannot interact */

  int optionIndex; /*!< Index for menu state options when at the prompt menu */
  bool promptOptions; /*!< Flag for menu state when selecting a folder to edit */
  bool enterText; /*!< Flag for user state when editting folders */

public:
  virtual void onStart();
  
  /**
   * @brief Responds to user input and menu states
   * @param elapsed in seconds
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void onUpdate(double elapsed);
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
<<<<<<< HEAD
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onEnd();

  FolderScene(swoosh::ActivityController&, ChipFolderCollection&);
=======
  
  /**
   * @brief Interpolate folder positions and draws
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Deletes all resources
   */
  virtual void onEnd();

  /**
   * @brief Requires the user's chip folder collection to display all chip folders
   * 
   * Loads and initializes all default graphics and state values
   */
  FolderScene(swoosh::ActivityController&, ChipFolderCollection&);
  
  /**
   * @brief deconstructor
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual ~FolderScene();
};
