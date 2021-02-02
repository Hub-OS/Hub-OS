#pragma once

#pragma once
#include <Swoosh/Activity.h>
#include <SFML/Graphics.hpp>
#include <time.h>

#include "bnScene.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"
#include "bnAnimation.h"
#include "bnConfigSettings.h"
#include "bnConfigWriter.h"
#include "bnAnimatedTextBox.h"
#include "bnMessageQuestion.h"
#include "bnFont.h"
#include "bnText.h"
#include "widgets/bnButton.h"

/*! \brief Config screen lets users set graphics, audio, and input settings. It also lets users manage their account.
    \warning This scene was made in a clear conscious and is in no way an example of good code design.

    This could use a redesign
*/
class Background;
class ConfigScene final : public Scene {
private:
  // scene states
  enum class states : unsigned {
    top_menu = 0,
    gamepad,
    keyboard,
    login
  } currState{ 0 };

  // member variables
  int audioModeBGM{};
  int audioModeSFX{};
  bool leave{};
  bool awaitingKey{};
  bool interactive{ false }; /*!< If false, player cannot interact with screen yet */
  ConfigSettings configSettings;
  ConfigSettings::KeyboardHash keyHash;
  ConfigSettings::GamepadHash gamepadHash;
  Animation endBtnAnimator;
  Animation audioAnimator;
  Animation lightAnimator;
  Text label;
  AnimatedTextBox textbox;
  sf::Sprite overlay; /*!< PET */
  sf::Sprite gba;
  sf::Sprite audioBGM,audioSFX;
  sf::Sprite hint;
  sf::Sprite endBtn;
  sf::Sprite authWidget, light;
  Background* bg{ nullptr };
  Question* questionInterface{ nullptr };
  Widget* menu{ nullptr };
#ifdef __ANDROID__
  void StartupTouchControls();
  void ShutdownTouchControls();
#endif

  /*
    the following use direct keyboard events as 
    opposed to config bindings because the 
    configuration may be invalid or the user
    wishes to change them so make this easy for
    them to do
    */
  const bool HasConfirmed() const;
  const bool HasCancelled() const;
  const bool HasUpButton() const;
  const bool HasDownButton() const;
  const bool HasLeftButton() const;
  const bool HasRightButton() const;
public:

  /**
   * @brief Load's the joystick config file
   */
  ConfigScene(swoosh::ActivityController&);

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
   * @brief Music fades out
   */
  void onLeave();

  /**
   * @brief Does nothing
   */
  void onExit();

  /**
   * @brief Does nothing
   */
  void onEnter();

  /**
   * @brief Does nothing
   */
  void onResume();

  /**
   * @brief Stops the music
   */
  void onEnd();

  /**
   * @brief deconstructor
   */
  ~ConfigScene();
};
