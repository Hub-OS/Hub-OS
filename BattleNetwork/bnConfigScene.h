#pragma once

#pragma once
#include <Swoosh/Activity.h>

#include "bnMainMenuScene.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include <SFML/Graphics.hpp>
#include <time.h>

class ConfigScene : public swoosh::Activity {
private:
  // ui sprite maps
  Animation uiAnimator; /*!< Use animator to represet the different UI buttons */
  Animation cursorAnimator;
  int menuSelectionIndex;; /*!< Current selection */
  int lastMenuSelectionIndex;
  int maxMenuSelectionIndex; 

  sf::Sprite overlay; /*!< PET */
  sf::Sprite gba;
  sf::Sprite audio;
  sf::Sprite hint;
  sf::Sprite cursor;

  bool leave;
  bool awaitingKey;
  int audioMode;
  
  Background* bg;

  sf::Sprite uiSprite;

  struct uiData {
    std::string label;
    sf::Vector2f position;
    sf::Vector2f scale;
    int alpha;
  };

  std::vector<uiData> uiList, boundKeys;

  std::vector<sf::Vector2f> points;

  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */

#ifdef __ANDROID__
  void StartupTouchControls();
  void ShutdownTouchControls();
#endif

public:

  /**
   * @brief Load's the joystick config file
   */
  ConfigScene(swoosh::ActivityController&);

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
   * @brief Music fades out
   */
  virtual void onLeave();

  /**
   * @brief Does nothing
   */
  virtual void onExit();

  /**
   * @brief Does nothing
   */
  virtual void onEnter();

  /**
   * @brief Does nothing
   */
  virtual void onResume();

  /**
   * @brief Stops the music
   */
  virtual void onEnd();

  /**
   * @brief deconstructor
   */
  virtual ~ConfigScene() { ; }
};
