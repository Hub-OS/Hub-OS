#pragma once
#include <Swoosh/Activity.h>

#include "bnFolderScene.h"
#include "bnOverworldMap.h"
#include "bnInfiniteMap.h"
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
#include "bnFont.h"

#include <SFML/Graphics.hpp>
#include <time.h>
#include <future>

class MainMenuScene : public swoosh::Activity {
private:
  Camera camera; /*!< camera in scene follows megaman */
  bool showHUD; /*!< Toggle HUD. Used in debugging. */

  // Selection input delays
  double maxSelectInputCooldown; /*!< Maximum delay */
  double selectInputCooldown; /*!< timer to allow key presses again */

  // ui sprite maps
  SpriteProxyNode ui; /*!< UI loaded as a texture atlas */
  Animation uiAnimator; /*!< Use animator to represent the different UI buttons */

  SpriteProxyNode webAccountIcon; /*!< Status icon if connected to web server*/
  Animation webAccountAnimator; /*!< Use animator to represent different statuses */
  bool lastIsConnectedState; /*!< Set different animations if the connection has changed */

  int menuSelectionIndex;; /*!< Current selection */
  int lastMenuSelectionIndex;

  SpriteProxyNode overlay; /*!< PET */
  SpriteProxyNode ow;

  Background* bg; /*!< Background image pointer */
  Overworld::Map* map; /*!< Overworld map pointer */ 

  SelectedNavi currentNavi; /*!< Current navi selection index */
  SpriteProxyNode owNavi; /*!< Overworld navi sprite */
  Animation naviAnimator; /*!< Animators navi sprite */
 
  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */

  CardFolderCollection data; /*!< TODO: this will be replaced with all saved data */

  std::future<WebAccounts::AccountState> accountCommandResponse; /*!< Response object that will wait for data from web server*/

#ifdef __ANDROID__
void StartupTouchControls();
void ShutdownTouchControls();
#endif

public:

  /**
   * @brief Loads the player's library data and loads graphics
   */
  MainMenuScene(swoosh::ActivityController&);
  
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
  
  /**
   * @brief deconstructor
   */
  virtual ~MainMenuScene() { ; }
};
