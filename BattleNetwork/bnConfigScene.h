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
  int primaryIndex{}; /*!< Current selection */
  int submenuIndex{};
  int lastMenuSelectionIndex;
  float scrollOffset;

  sf::Sprite endBtn;

  bool leave{}; // ?
  bool awaitingKey{};
  bool isSelectingTopMenu{};
  bool inGamepadList{};
  bool inKeyboardList{};
  bool inLoginMenu{};

  Background* bg{ nullptr };

  class MenuItem : public SceneNode {
  private:
    std::function<void()> action;
  public:
    MenuItem(const std::function<void()>& callback) { action = callback; }
    virtual void SetAlpha(sf::Uint8) = 0;
    void Select() { action(); };
    virtual void Update() {};
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
  };

  class TextItem : public MenuItem {
  private:
    Text label;
    sf::Color color;
  public:
    TextItem(const std::string& text, const std::function<void()>& callback);
    const std::string& GetString();
    void SetString(const std::string& text);
    void SetColor(sf::Color);
    void SetAlpha(sf::Uint8) override;
  };

  class LoginItem : public TextItem {
  public:
    LoginItem(const std::function<void()>& callback);
    void Update() override;
  };

  class BindingItem : public TextItem {
  private:
    Text value;
    sf::Color valueColor;
  public:
    BindingItem(
      const std::string& inputName,
      std::optional<std::reference_wrapper<std::string>> valueName,
      const std::function<void(BindingItem&)>& callback
    );
    void SetValue(std::optional<std::reference_wrapper<std::string>> valueName);
    void SetAlpha(sf::Uint8 alpha) override;
    void Update() override;
  };

  class VolumeItem : public MenuItem, ResourceHandle {
  private:
    SpriteProxyNode icon;
    Animation animator;
    sf::Color color;
    int volumeLevel{};
    std::function<void()> createCallback(const std::function<void(int)>&);
  public:
    VolumeItem(sf::Color color, int volumeLevel, const std::function<void(int)>& callback);
    void SetAlpha(sf::Uint8 alpha) override;
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
    } currState{ states::complete };
  } user;

  int menuDivideIndex;

  std::vector<std::unique_ptr<MenuItem>> primaryMenu;
  std::vector<std::unique_ptr<BindingItem>> keyboardMenu, gamepadMenu;

  Question* questionInterface{ nullptr };
  MessageInput* inputInterface{ nullptr };

#ifdef __ANDROID__
  void StartupTouchControls();
  void ShutdownTouchControls();
#endif
  void UpdateBgmVolume(int volumeLevel);
  void UpdateSfxVolume(int volumeLevel);
  void ToggleShaders();
  void ShowKeyboardMenu();
  void ShowGamepadMenu();
  void ToggleLogin();
  void LoginStep(UserInfo::states next);
  void AwaitKeyBinding(BindingItem&);
  void AwaitGamepadBinding(BindingItem&);

  bool IsInSubmenu();
  void UpdateMenuItem(MenuItem&, bool menuHasFocus, int index, int selectionIndex, float colSpan, float elapsed);

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
