#pragma once
#include "overworld/bnOverworldHomepage.h"
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
#include "bnMessageInput.h"
#include <SFML/Graphics.hpp>
#include <time.h>

/*! \brief Config screen lets users set graphics, audio, and input settings. It also lets users manage their account.
    \warning This scene was made in a clear conscious and is in no way an example of good code design.

    This could use a redesign (and re-code)
*/
class ConfigScene : public Scene {
private:
  ConfigSettings configSettings;
  ConfigSettings::KeyboardHash keyHash;
  ConfigSettings::GamepadHash gamepadHash;

  AnimatedTextBox textbox;

  // ui sprite maps
  Animation endBtnAnimator;
  Animation audioAnimator;
  int menuSelectionIndex;; /*!< Current selection */
  int lastMenuSelectionIndex;
  int maxMenuSelectionIndex;
  int colIndex;
  int maxCols;

  sf::Sprite overlay; /*!< PET */
  sf::Sprite gba;
  SpriteProxyNode audioBGM, audioSFX;
  sf::Sprite hint;
  sf::Sprite endBtn;

  bool leave; // ?
  bool awaitingKey;
  bool isSelectingTopMenu;
  bool inGamepadList;
  bool inKeyboardList;
  bool inLoginMenu;
  int audioModeBGM;
  int audioModeSFX;

  Background* bg{ nullptr };

  struct uiData {
    std::string label;
    sf::Vector2f position;
    sf::Vector2f scale;
    enum class ActionItemType : int {
      KEYBOARD,
      GAMEPAD,
      DISABLED
    } type;
    int alpha;

    uiData() = default;
    uiData(const uiData& rhs) = default;
    ~uiData() = default;
  };

  struct UserInfo {
    std::string username;
    std::string password;
    std::future<bool> result;
    enum class states : char {
      entering_username,
      entering_password,
      pending,
      complete
    } currState{states::complete};
  } user;

  int menuDivideIndex;

  std::vector<uiData> uiList[3], boundKeys, boundGamepadButtons;

  bool gotoNextScene; /*!< If true, player cannot interact with screen yet */

  Question* questionInterface{ nullptr };
  MessageInput* inputInterface{ nullptr };

#ifdef __ANDROID__
  void StartupTouchControls();
  void ShutdownTouchControls();
#endif
  void DrawMenuOptions(sf::RenderTexture& surface);
  void DrawMappedKeyMenu(std::vector<uiData>& container, sf::RenderTexture& surface);
  void LoginStep(UserInfo::states next);

public:

  /**
   * @brief Load's the joystick config file
   */
  ConfigScene(swoosh::ActivityController&);
  ~ConfigScene();

  /**
   * @brief Checks input events and listens for select buttons. Segues to new screens.
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed) override;

  /**
   * @brief Draws the UI
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface) override;

  /**
   * @brief Stops music, plays new track, reset the camera
   */
  void onStart() override;

  /**
   * @brief Music fades out
   */
  void onLeave() override;

  /**
   * @brief Does nothing
   */
  void onExit() override;

  /**
   * @brief Does nothing
   */
  void onEnter() override;

  /**
   * @brief Does nothing
   */
  void onResume() override;

  /**
   * @brief Stops the music
   */
  void onEnd() override;
};
