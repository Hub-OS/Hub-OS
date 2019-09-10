#pragma once
#include <Swoosh/Activity.h>

#include "bnFolderScene.h"
#include "bnOverworldMap.h"
#include "bnInfiniteMap.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnLibraryScene.h"
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
#include "bnChipFolderCollection.h"
#include <SFML/Graphics.hpp>
#include <time.h>

class MainMenuScene : public swoosh::Activity {
private:
  Camera camera; /*!< camera in scene follows megaman */
  bool showHUD; /*!< Toggle HUD. Used in debugging. */

  // Selection input delays
  double maxSelectInputCooldown; /*!< Maximum delay */
  double selectInputCooldown; /*!< timer to allow key presses again */

  // ui sprite maps
  sf::Sprite ui; /*!< UI loaded as a texture atlas */
  Animation uiAnimator; /*!< Use animator to represet the different UI buttons */

  int menuSelectionIndex;; /*!< Current selection */
  int lastMenuSelectionIndex;

  sf::Sprite overlay; /*!< PET */
  sf::Sprite ow; 

  Background* bg; /*!< Background image pointer */
  Overworld::Map* map; /*!< Overworld map pointer */ 

  SelectedNavi currentNavi; /*!< Current navi selection index */
  sf::Sprite owNavi; /*!< Overworld navi sprite */
  Animation naviAnimator; /*!< Animators navi sprite */
 
  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */

  ChipFolderCollection data; /*!< TODO: this will be replaced with all saved data */

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
  virtual void onUpdate(double elapsed);
  
  /**
   * @brief Draws the UI
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Stops music, plays new track, reset the camera
   */
  virtual void onStart();
  
  /**
   * @brief Does nothing
   */
  virtual void onLeave();
  
  /**
   * @brief Does nothing
   */
  virtual void onExit();
  
  /**
   * @brief Checks the selected navi if changed and loads the new images
   */
  virtual void onEnter();
  
  /**
   * @brief Resets the camera
   */
  virtual void onResume();
  
  /**
   * @brief Stops the music
   */
  virtual void onEnd();
  
  /**
   * @brief deconstructor
   */
  virtual ~MainMenuScene() { ; }
};
