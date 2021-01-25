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

/*! \brief Config screen lets users set graphics, audio, and input settings. It also lets users manage their account.
    \warning This scene was made in a clear conscious and is in no way an example of good code design.

    This could use a redesign
*/
class Background;
class ConfigScene : public Scene {
private:
  ConfigSettings configSettings;
  ConfigSettings::KeyboardHash keyHash;
  ConfigSettings::GamepadHash gamepadHash;

  AnimatedTextBox textbox;

  // ui sprite maps
  Animation endBtnAnimator;
  Animation audioAnimator;
  Text label;
  int menuSelectionIndex{}; /*!< Current selection */
  int lastMenuSelectionIndex{};
  int maxMenuSelectionIndex{};
  int colIndex{};
  int maxCols{};

  sf::Sprite overlay; /*!< PET */
  sf::Sprite gba;
  sf::Sprite audioBGM,audioSFX;
  sf::Sprite hint;
  sf::Sprite endBtn;

  bool leave{};
  bool awaitingKey{};
  bool isSelectingTopMenu{ true };
  bool inGamepadList{};
  bool inKeyboardList{};
  int audioModeBGM{};
  int audioModeSFX{};

  Background* bg;

  struct uiData {
    std::string label;
    sf::Vector2f position;
    sf::Vector2f scale;
    enum class ActionItemType : int {
      keyboard,
      gamepad,
      disabled
    } type;
    int alpha{255};

    uiData() = default;
    uiData(const uiData& rhs) = default;
    ~uiData() = default;
  };

  enum class State : unsigned char {
    menu = 0,
    gamepad_select,
    login
  } currState{ State::menu };

  int menuDivideIndex;

  std::vector<uiData> uiList[3], boundKeys, boundGamepadButtons;

  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */

  Question* questionInterface;

#ifdef __ANDROID__
  void StartupTouchControls();
  void ShutdownTouchControls();
#endif
  void DrawMenuOptions(sf::RenderTarget& surface);
  void DrawMappedKeyMenu(std::vector<uiData>& container, sf::RenderTarget& surface);

  void DrawMenuState(sf::RenderTarget& surface);
  void UpdateMenuState(double elapsed);

  void DrawGamepadState(sf::RenderTarget& surface);
  void UpdateGamepadState(double elapsed);

  void DrawLoginState(sf::RenderTarget& surface);
  void UpdateLoginState(double elapsed);

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
  virtual ~ConfigScene() { ; }
};
