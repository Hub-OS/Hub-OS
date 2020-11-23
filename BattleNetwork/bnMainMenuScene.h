#pragma once
#include <Swoosh/Activity.h>

#include "bnFolderScene.h"
#include "bnMenuWidget.h"
#include "bnOverworldMap.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnLibraryScene.h"
#include "bnConfigScene.h"
#include "bnFolderScene.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnCardFolderCollection.h"
#include "bnAnimatedTextBox.h"

// overworld
#include "overworld/bnOverworldActor.h"
#include "overworld/bnOverworldPlayerController.h"

#include <SFML/Graphics.hpp>
#include <time.h>
#include <future>

class MainMenuScene : public swoosh::Activity {
private:
  Overworld::Actor actor{ "Test" };
  Overworld::PlayerController playerController{};

  Camera camera; /*!< camera in scene follows megaman */
  bool showHUD; /*!< Toggle HUD. Used in debugging. */

  // Selection input delays
  double maxSelectInputCooldown; /*!< Maximum delay */
  double selectInputCooldown; /*!< timer to allow key presses again */
  bool extendedHold{ false };

  // menu widget
  MenuWidget menuWidget;

  SpriteProxyNode webAccountIcon; /*!< Status icon if connected to web server*/
  Animation webAccountAnimator; /*!< Use animator to represent different statuses */
  bool lastIsConnectedState; /*!< Set different animations if the connection has changed */

  Background* bg; /*!< Background image pointer */
  Overworld::Map map; /*!< Overworld map */ 
  Overworld::Map::Tile** tiles{ nullptr }; /*!< Loaded tiles from file */
  std::vector<std::shared_ptr<SpriteProxyNode>> trees;
  Animation treeAnim;
  std::shared_ptr<sf::Texture> treeTexture;
  std::vector<std::shared_ptr<SpriteProxyNode>> warps;
  Animation warpAnim;
  std::shared_ptr<sf::Texture> warpTexture;
  std::vector<std::shared_ptr<SpriteProxyNode>> coffees;
  Animation coffeeAnim;
  std::shared_ptr<sf::Texture> coffeeTexture;
  std::vector<std::shared_ptr<SpriteProxyNode>> gates;
  Animation gateAnim;
  std::shared_ptr<sf::Texture> gateTexture;

  AnimatedTextBox textbox;

  double animElapsed{};

  SelectedNavi currentNavi; /*!< Current navi selection index */
 
  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */
  bool guestAccount;

  CardFolderCollection folders; /*!< Collection of folders */
  PA programAdvance;

  std::future<WebAccounts::AccountState> accountCommandResponse; /*!< Response object that will wait for data from web server*/

  void RefreshNaviSprite();

  /**
  * @brief Equip a folder for the navi that was last used when playing as them
  */
  void NaviEquipSelectedFolder();

  void ResetMap();

#ifdef __ANDROID__
void StartupTouchControls();
void ShutdownTouchControls();
#endif

public:

  /**
   * @brief Loads the player's library data and loads graphics
   */
  MainMenuScene(swoosh::ActivityController&, bool guestAccount);
  
  /**
  * @brief deconstructor
  */
  ~MainMenuScene() { ; }

  /**
   * @brief Checks input events and listens for select buttons. Segues to new screens.
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed);
  
  /**
   * @brief Draws the UI
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Stops music, plays new track, reset the camera
   */
  void onStart();
  
  /**
   * @brief Does nothing
   */
  void onLeave();
  
  /**
   * @brief Does nothing
   */
  void onExit();
  
  /**
   * @brief Checks the selected navi if changed and loads the new images
   */
  void onEnter();
  
  /**
   * @brief Resets the camera
   */
  void onResume();
  
  /**
   * @brief Stops the music
   */
  void onEnd();
  
  //
  // Menu selection callbacks
  //

  void GotoChipFolder();
  void GotoNaviSelect();
  void GotoConfig();
  void GotoMobSelect();
  void GotoPVP();
};
