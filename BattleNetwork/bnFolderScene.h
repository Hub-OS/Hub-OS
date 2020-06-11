#pragma once
#include <Swoosh/Ease.h>
#include <Swoosh/Activity.h>

#include "bnCamera.h"
#include "bnCardFolderCollection.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnAnimatedTextBox.h"

/**
 * @class FolderScene
 * @author mav
 * @date 04/05/19
 * @brief Display folders at top and contents in the center. Player may add, edit, or remove folders.
 * 
 * Use input keys LEFT/RIGHT to navigate through infinite list of folders
 * Use input keys UP/DOWN to scroll through cards inside the folders
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
  CardFolderCollection& collection; /*!< The entire user collection */
  CardFolder* folder; /*!< Handle to current folder to preview */
  std::vector<std::string> folderNames; /*!< List of all folder names at start */

  // Menu name font
  std::shared_ptr<sf::Font> font; /*!< Font of the  menu name label*/
  sf::Text* menuLabel; /*!< "Folder" text on top-left */

  // Selection input delays
  double maxSelectInputCooldown; /*!< Set to half of a second */
  double selectInputCooldown; /*!< The delay between reading user input */

  // Card UI font
  std::shared_ptr<sf::Font> cardFont;
  sf::Text *cardLabel;

  std::shared_ptr<sf::Font> numberFont;
  sf::Text *numberLabel;

  // folder menu graphics
  sf::Sprite bg;
  sf::Sprite folderBox;
  sf::Sprite folderCursor;
  sf::Sprite scrollbar;
  sf::Sprite folderOptions;
  sf::Sprite element;
  sf::Sprite cursor;
  sf::Sprite folderEquip;
  sf::Sprite cardIcon;
  sf::Sprite mbPlaceholder;

  Animation equipAnimation; /*!< Flashes */
  Animation folderCursorAnimation; /*!< Flashes */

  AnimatedTextBox textbox;

  int currFolderIndex;
  int selectedFolderIndex;
  int lastFolderIndex;

  int maxCardsOnScreen; /*!< The number of cards to display max in box area */
  int currCardIndex; /*!< Current index in current folder */
  int numOfCards; /*!< Number of cards in current folder */

  double totalTimeElapsed;
  double frameElapsed;
  double folderOffsetX;

  bool gotoNextScene; /*!< If true, user cannot interact */
  bool folderSwitch; /*!< If a folder at currIndex was changed or index was changed*/

  int optionIndex; /*!< Index for menu state options when at the prompt menu */
  bool promptOptions; /*!< Flag for menu state when selecting a folder to edit */
  bool enterText; /*!< Flag for user state when editting folders */

#ifdef __ANDROID__
    bool canSwipe;
    bool touchStart;

    int touchPosX;
    int touchPosStartX;

    bool releasedB;

    void StartupTouchControls();
    void ShutdownTouchControls();
#endif

  void MakeNewFolder();
  void DeleteFolder(std::function<void()> onSuccess);

public:  

  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onStart() override;
  
  /**
  * @brief Responds to user input and menu states
  * @param elapsed in seconds
  */
  void onUpdate(double elapsed) override;
  
  /**
   * @brief Interpolate folder positions and draws
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface) override;
  
  /**
   * @brief Deletes all resources
   */
  void onEnd() override;

  /**
   * @brief Requires the user's card folder collection to display all card folders
   * 
   * Loads and initializes all default graphics and state values
   */
  FolderScene(swoosh::ActivityController&, CardFolderCollection&);
  
  /**
   * @brief deconstructor
   */
  ~FolderScene();
};
