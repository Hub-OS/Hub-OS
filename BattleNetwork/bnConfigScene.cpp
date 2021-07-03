#include "bnConfigScene.h"
#include "bnWebClientMananger.h"
#include "Segues/WhiteWashFade.h"
#include "bnRobotBackground.h"

#undef GetUserName

constexpr float COL_PADDING = 4.0f;
constexpr float SUBMENU_SPAN = 90.0f;
constexpr float BINDED_VALUE_OFFSET = 240.0f - SUBMENU_SPAN - COL_PADDING;
constexpr float MENU_START_Y = 40.0f;
constexpr float LINE_SPAN = 15.0f;
constexpr float SCROLL_INTERPOLATION_MULTIPLIER = 6.0f;

const sf::Color DEFAULT_TEXT_COLOR = sf::Color(255, 165, 0);
const sf::Color DISABLED_TEXT_COLOR = sf::Color(255, 0, 0);
const sf::Color NO_BINDING_COLOR = sf::Color(10, 165, 255);

void ConfigScene::MenuItem::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  // SceneNode doesn't apply transform like SpriteProxyNode
  states.transform *= getTransform();
  SceneNode::draw(target, states);
}

ConfigScene::TextItem::TextItem(
  const std::string& text,
  const std::function<void()>& callback
) :
  MenuItem(callback),
  label(text, Font::Style::wide),
  color(DEFAULT_TEXT_COLOR)
{
  AddNode(&label);
}

const std::string& ConfigScene::TextItem::GetString() {
  return label.GetString();
}

void ConfigScene::TextItem::SetString(const std::string& text) {
  label.SetString(text);
}

void ConfigScene::TextItem::SetColor(sf::Color color) {
  this->color = color;
  label.SetColor(color);
}

void ConfigScene::TextItem::SetAlpha(sf::Uint8 alpha) {
  color.a = alpha;
  label.SetColor(color);
}

ConfigScene::LoginItem::LoginItem(const std::function<void()>& callback) : TextItem("Login", callback) {}

void ConfigScene::LoginItem::Update() {
  if (!WEBCLIENT.IsLoggedIn()) {
    SetString("LOGIN");
  }
  else {
    SetString("LOGOUT " + WEBCLIENT.GetUserName());
  }
}

ConfigScene::BindingItem::BindingItem(
  const std::string& inputName,
  std::optional<std::reference_wrapper<std::string>> valueName,
  const std::function<void(BindingItem&)>& callback
) :
  TextItem(inputName, [this, callback] { callback(*this); }),
  value(Font::Style::wide)
{
  SetValue(valueName);
  AddNode(&value);
}

void ConfigScene::BindingItem::SetValue(std::optional<std::reference_wrapper<std::string>> valueName) {
  if (valueName) {
    valueColor = DEFAULT_TEXT_COLOR;
    value.SetString(valueName->get());
  }
  else {
    valueColor = NO_BINDING_COLOR;
    value.SetString("NO KEY");
  }
}

void ConfigScene::BindingItem::SetAlpha(sf::Uint8 alpha) {
  valueColor.a = alpha;
  value.SetColor(valueColor);
  TextItem::SetAlpha(alpha);
}

void ConfigScene::BindingItem::Update() {
  value.setPosition((BINDED_VALUE_OFFSET * 2.0f) / getScale().x - value.GetLocalBounds().width, 0.0f);
}

ConfigScene::VolumeItem::VolumeItem(sf::Color color, int volumeLevel, const std::function<void(int)>& callback) :
  MenuItem(createCallback(callback)),
  color(color),
  volumeLevel(volumeLevel)
{
  AddNode(&icon);
  icon.setTexture(Textures().LoadTextureFromFile("resources/ui/config/audio.png"));

  animator = Animation("resources/ui/config/audio.animation");
  animator.Load();
  animator.SetAnimation("DEFAULT");
  animator.SetFrame(volumeLevel + 1, icon.getSprite());
  animator.Update(0, icon.getSprite());
}

std::function<void()> ConfigScene::VolumeItem::createCallback(const std::function<void(int)>& callback) {
  return [this, callback] {
    volumeLevel = (volumeLevel + 1) % 4;
    animator.SetFrame(volumeLevel + 1, icon.getSprite());
    callback(volumeLevel);
  };
}

void ConfigScene::VolumeItem::SetAlpha(sf::Uint8 alpha) {
  color.a = alpha;
  icon.setColor(color);
}

ConfigScene::ConfigScene(swoosh::ActivityController& controller) :
  textbox(sf::Vector2f(4, 250)),
  Scene(controller)
{
  textbox.SetTextSpeed(2.0);
  isSelectingTopMenu = inGamepadList = inKeyboardList = false;

  // Draws the scrolling background
  bg = new RobotBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  endBtnAnimator = Animation("resources/ui/config/end_btn.animation");
  endBtnAnimator.Load();

  // end button
  endBtn = sf::Sprite(*LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2 * 180, 2 * 10);

  // ui sprite maps
  // ascii 58 - 96
  primaryMenu.push_back(std::make_unique<VolumeItem>(sf::Color(255, 0, 255), configSettings.GetMusicLevel(), [this](int volumeLevel) { UpdateBgmVolume(volumeLevel); }));
  primaryMenu.push_back(std::make_unique<VolumeItem>(sf::Color(10, 165, 255), configSettings.GetSFXLevel(), [this](int volumeLevel) { UpdateSfxVolume(volumeLevel); }));

  auto shadersItem = std::make_unique<TextItem>("SHADERS: ON", [this] { ToggleShaders(); });
  shadersItem->SetColor(DISABLED_TEXT_COLOR);
  primaryMenu.push_back(std::move(shadersItem));

  primaryMenu.push_back(std::make_unique<TextItem>("MY KEYBOARD", [this] { ShowKeyboardMenu(); }));
  primaryMenu.push_back(std::make_unique<TextItem>("MY GAMEPAD", [this] { ShowGamepadMenu(); }));
  primaryMenu.push_back(std::make_unique<LoginItem>([this] { ToggleLogin(); }));

  configSettings = Input().GetConfigSettings();

  // For keyboard keys 
  for (auto eventName : InputEvents::KEYS) {
    auto callback = [this](BindingItem& item) { AwaitKeyBinding(item); };

    if (configSettings.IsOK()) {
      std::optional<std::reference_wrapper<std::string>> value;
      std::string keyStr;

      if (Input().ConvertKeyToString(configSettings.GetPairedInput(eventName), keyStr)) {
        keyHash.insert(std::make_pair(configSettings.GetPairedInput(eventName), eventName));
        value = keyStr;
      }

      keyboardMenu.push_back(std::make_unique<BindingItem>(eventName, value, callback));
    }
  }

  // For gamepad keys

  for (auto eventName : InputEvents::KEYS) {
    std::optional<std::reference_wrapper<std::string>> value;

    if (configSettings.IsOK()) {
      auto gamepadCode = configSettings.GetPairedGamepadButton(eventName);
      gamepadHash.insert(std::make_pair(gamepadCode, eventName));

      if (gamepadCode != Gamepad::BAD_CODE) {
        std::string valueString = "BTN " + std::to_string((int)gamepadCode);

        switch (gamepadCode) {
        case Gamepad::DOWN:
          valueString = "-Y AXIS";
          break;
        case Gamepad::UP:
          valueString = "+Y AXIS";
          break;
        case Gamepad::LEFT:
          valueString = "-X AXIS";
          break;
        case Gamepad::RIGHT:
          valueString = "+X AXIS";
          break;
        case Gamepad::BAD_CODE:
          valueString = "BAD_CODE";
          break;
        }

        value = valueString;
      }
    }

    auto callback = [this](BindingItem& item) { AwaitGamepadBinding(item); };
    gamepadMenu.push_back(std::make_unique<BindingItem>(eventName, value, callback));
  }

  setView(sf::Vector2u(480, 320));
}

ConfigScene::~ConfigScene() { }

void ConfigScene::UpdateBgmVolume(int volumeLevel) {
  Audio().SetStreamVolume((volumeLevel / 3.0f) * 100.0f);
  configSettings.SetMusicLevel(volumeLevel);
}

void ConfigScene::UpdateSfxVolume(int volumeLevel) {
  Audio().SetChannelVolume((volumeLevel / 3.0f) * 100.0f);
  Audio().Play(AudioType::BUSTER_PEA);
  configSettings.SetSFXLevel(volumeLevel);
}

void ConfigScene::ToggleShaders() {
  // TODO: Shader Toggle
  Audio().Play(AudioType::CHIP_ERROR);
}

void ConfigScene::ShowKeyboardMenu() {
  inKeyboardList = true;
  Audio().Play(AudioType::CHIP_SELECT);
}

void ConfigScene::ShowGamepadMenu() {
  inGamepadList = true;
  Audio().Play(AudioType::CHIP_SELECT);
}

void ConfigScene::ToggleLogin() {
  if (WEBCLIENT.IsLoggedIn()) {
    if (textbox.IsClosed()) {
      auto onYes = [this]() {
        Logger::Log("SendLogoutCommand");
        WEBCLIENT.SendLogoutCommand();
        textbox.Close();
      };

      auto onNo = [this]() {
        textbox.Close();
        Audio().Play(AudioType::CHIP_DESC_CLOSE);
      };
      questionInterface = new Question("Are you sure you want to logout?", onYes, onNo);
      textbox.EnqueMessage(questionInterface);
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
  }
  else {
    LoginStep(UserInfo::states::entering_username);
  }
}

void ConfigScene::AwaitKeyBinding(BindingItem& item) {
  awaitingKey = true;
}

void ConfigScene::AwaitGamepadBinding(BindingItem& item) {
  awaitingKey = true;
}

bool ConfigScene::IsInSubmenu() {
  return inKeyboardList || inGamepadList;
}

void ConfigScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);
  bg->Update((float)elapsed);

  if (user.currState == UserInfo::states::pending) {
    if (is_ready(user.result)) {
      if (user.result.get()) {
        LoginStep(UserInfo::states::complete);
      }
      else {
        user.currState = UserInfo::states::entering_username;
        user.password.clear();
        user.username.clear();

        Audio().Play(AudioType::CHIP_ERROR);
      }
    }

    return;
  }

  bool hasConfirmed = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_confirm) : false) || Input().GetAnyKey() == sf::Keyboard::Enter;

  if (hasConfirmed && isSelectingTopMenu && !leave) {
    if (textbox.IsClosed()) {
      auto onYes = [this]() {
        // Save before leaving
        configSettings.SetKeyboardHash(keyHash);
        configSettings.SetGamepadHash(gamepadHash);

        ConfigWriter writer(configSettings);
        writer.Write("config.ini");
        ConfigReader reader("config.ini");
        Input().SupportConfigSettings(reader);
        textbox.Close();

        // transition to the next screen
        using namespace swoosh::types;
        using effect = segue<WhiteWashFade, milliseconds<300>>;
        getController().pop<effect>();

        Audio().Play(AudioType::NEW_GAME);
        leave = true;
      };

      auto onNo = [this]() {
        // Just close and leave
        using namespace swoosh::types;
        using effect = segue<WhiteWashFade, milliseconds<300>>;
        getController().pop<effect>();
        leave = true;

        textbox.Close();
      };
      questionInterface = new Question("Overwite your config settings?", onYes, onNo);
      textbox.EnqueMessage(sf::Sprite(), "", questionInterface);
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
  }

  if (!leave) {
    bool hasConfirmed = Input().GetAnyKey() == sf::Keyboard::Return;

    bool hasCanceled = (Input().GetAnyKey() == sf::Keyboard::BackSpace || Input().GetAnyKey() == sf::Keyboard::Escape);

    bool hasUp = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_ui_up) : false) || Input().GetAnyKey() == sf::Keyboard::Up;
    bool hasDown = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_ui_down) : false) || Input().GetAnyKey() == sf::Keyboard::Down;
    bool hasLeft = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_ui_left) : false) || Input().GetAnyKey() == sf::Keyboard::Left;
    bool hasRight = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_ui_right) : false) || Input().GetAnyKey() == sf::Keyboard::Right;

    if (textbox.IsOpen()) {
      if (questionInterface) {
        if (textbox.IsEndOfMessage()) {
          if (hasLeft) {
            questionInterface->SelectYes();
          }
          else if (hasRight) {
            questionInterface->SelectNo();
          }
          else if (hasCanceled) {
            questionInterface->Cancel();
          }
          else if (hasConfirmed) {
            questionInterface->ConfirmSelection();
          }
        }
        else if (hasConfirmed) {
          if (!textbox.IsPlaying()) {
            questionInterface->Continue();
          }
          else {
            textbox.CompleteCurrentBlock();
          }
        }
      }
      else if (inputInterface) {
        if (inputInterface->IsDone()) {
          std::string entry = inputInterface->Submit();
          textbox.Close();

          inputInterface = nullptr;

          switch (user.currState) {
          case UserInfo::states::entering_username:
            user.username = entry;
            LoginStep(UserInfo::states::entering_password);
            break;
          case UserInfo::states::entering_password:
            user.password = entry;
            user.result = WEBCLIENT.SendLoginCommand(user.username, user.password);
            LoginStep(UserInfo::states::pending);
            break;
          }
        }
      }
    }
    else if (hasCanceled && !awaitingKey) {
      if (!IsInSubmenu()) {
        Audio().Play(AudioType::CHIP_DESC_CLOSE);
      }
      else {
        inGamepadList = inKeyboardList = false;
        submenuIndex = 0;

        Audio().Play(AudioType::CHIP_DESC_CLOSE);
      }
    }
    else if (awaitingKey) {
      if (inKeyboardList) {
        auto key = Input().GetAnyKey();

        if (key != sf::Keyboard::Unknown) {
          std::string boundKey = "";

          if (Input().ConvertKeyToString(key, boundKey)) {
            auto iter = keyHash.begin();

            while (iter != keyHash.end()) {
              if (iter->second == InputEvents::KEYS[submenuIndex]) break;
              iter++;
            }

            if (iter != keyHash.end()) {
              keyHash.erase(iter);
            }

            keyHash.insert(std::make_pair(key, InputEvents::KEYS[submenuIndex]));

            std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);
            keyboardMenu[submenuIndex]->SetValue(boundKey);
            Audio().Play(AudioType::CHIP_DESC_CLOSE);

            awaitingKey = false;
          }
        }
      }

      if (inGamepadList) {
        // GAMEPAD
        auto gamepad = Input().GetAnyGamepadButton();

        if (gamepad != (Gamepad)-1) {
          auto iter = gamepadHash.begin();

          while (iter != gamepadHash.end()) {
            if (iter->second == InputEvents::KEYS[submenuIndex]) break;
            iter++;
          }

          if (iter != gamepadHash.end()) {
            gamepadHash.erase(iter);
          }

          gamepadHash.insert(std::make_pair(gamepad, InputEvents::KEYS[submenuIndex]));

          std::string label = "BTN " + std::to_string((int)gamepad);

          switch (gamepad) {
          case Gamepad::DOWN:
            label = "-Y AXIS";
            break;
          case Gamepad::UP:
            label = "+Y AXIS";
            break;
          case Gamepad::LEFT:
            label = "-X AXIS";
            break;
          case Gamepad::RIGHT:
            label = "+X AXIS";
            break;
          case Gamepad::BAD_CODE:
            label = "BAD_CODE";
            break;
          }

          gamepadMenu[submenuIndex]->SetValue(label);

          Audio().Play(AudioType::CHIP_DESC_CLOSE);

          awaitingKey = false;
        }
      }
    }
    else if (hasUp && textbox.IsClosed()) {
      if (IsInSubmenu()) {
        submenuIndex--;
      }
      else {
        if (primaryIndex == 0 && !isSelectingTopMenu) {
          isSelectingTopMenu = true;
        }
        else {
          primaryIndex--;
        }
      }

      Audio().Play(AudioType::CHIP_SELECT);
    }
    else if (hasDown && textbox.IsClosed()) {
      if (IsInSubmenu()) {
        submenuIndex++;
      }
      else {
        if (isSelectingTopMenu) {
          isSelectingTopMenu = false;
        }
        else {
          primaryIndex++;
        }
      }

      Audio().Play(AudioType::CHIP_SELECT);
    }
    else if (hasLeft) {
      // unused
    }
    else if (hasRight) {
      // unused
    }
    else if (hasConfirmed && !isSelectingTopMenu) {
      if (IsInSubmenu()) {
        auto& submenu = inKeyboardList ? keyboardMenu : gamepadMenu;
        submenu[submenuIndex]->Select();
      }
      else {
        primaryMenu[primaryIndex]->Select();
      }
    }
  }

  primaryIndex = std::clamp(primaryIndex, 0, int(primaryMenu.size()) - 1);

  for (int i = 0; i < primaryMenu.size(); i++) {
    UpdateMenuItem(*primaryMenu[i], !IsInSubmenu(), i, primaryIndex, 0, float(elapsed));
  }

  // display submenu, currently just keyboard/gamepad binding
  if (IsInSubmenu()) {
    auto& submenu = inKeyboardList ? keyboardMenu : gamepadMenu;
    submenuIndex = std::clamp(submenuIndex, 0, int(submenu.size()) - 1);

    for (int i = 0; i < submenu.size(); i++) {
      UpdateMenuItem(*submenu[i], true, i, submenuIndex, SUBMENU_SPAN, float(elapsed));
    }
  }

  // Make endBtn stick at the top of the screen
  endBtn.setPosition(endBtn.getPosition().x, primaryMenu[0]->getPosition().y - 60);

  if (isSelectingTopMenu) {
    endBtnAnimator.SetFrame(2, endBtn);
  }
  else {
    endBtnAnimator.SetFrame(1, endBtn);
  }

  // move view based on selection (keep selection in view)
  auto menuSize = primaryMenu.size();
  auto selectionIndex = primaryIndex;

  if (IsInSubmenu()) {
    auto& submenu = inKeyboardList ? keyboardMenu : gamepadMenu;
    menuSize = submenu.size();
    selectionIndex = submenuIndex;
  }

  auto scrollEnd = std::max(0.0f, float(menuSize + 1) * LINE_SPAN - getView().getSize().y / 2.0f + MENU_START_Y);
  auto scrollIncrement = scrollEnd / float(menuSize);
  auto newScrollOffset = -float(selectionIndex) * scrollIncrement;
  scrollOffset = swoosh::ease::interpolate(float(elapsed) * SCROLL_INTERPOLATION_MULTIPLIER, scrollOffset, newScrollOffset);
}

void ConfigScene::UpdateMenuItem(MenuItem& menuItem, bool menuHasFocus, int index, int selectionIndex, float offsetX, float elapsed) {
  auto w = 0.3f;
  auto diff = index - selectionIndex;
  float scale = 1.0f - (w * abs(diff));
  scale = std::max(scale, 0.8f);
  scale = std::min(scale, 1.0f);

  auto delta = 48.0f * elapsed;

  auto s = sf::Vector2f(2.f * scale, 2.f * scale);
  auto menuItemScale = menuItem.getScale().x;
  auto slerp = sf::Vector2f(
    swoosh::ease::interpolate(delta, s.x, menuItemScale),
    swoosh::ease::interpolate(delta, s.y, menuItemScale)
  );
  menuItem.setScale(slerp);

  menuItem.setPosition(COL_PADDING + 2.f * offsetX, 2.f * (MENU_START_Y + (index * LINE_SPAN)));

  if (menuHasFocus) {
    if (index == selectionIndex) {
      menuItem.SetAlpha(255);
    }
    else {
      menuItem.SetAlpha(awaitingKey ? 50 : 150);
    }
  }
  else {
    menuItem.SetAlpha(awaitingKey ? 50 : 100);
  }

  menuItem.Update();
}

void ConfigScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(*bg);

  // scrolling the view
  auto states = sf::RenderStates::Default;
  states.transform.translate(0, 2.0f * scrollOffset);

  surface.draw(endBtn, states);

  // Draw options
  for (auto& menuItem : primaryMenu) {
    surface.draw(*menuItem, states);
  }

  if (inKeyboardList) {
    // Keyboard keys
    for (auto& menuItem : keyboardMenu) {
      surface.draw(*menuItem, states);
    }
  }

  if (inGamepadList) {
    // Gamepad keys
    for (auto& menuItem : gamepadMenu) {
      surface.draw(*menuItem, states);
    }
  }

  surface.draw(textbox);
}

void ConfigScene::LoginStep(UserInfo::states next)
{
  if (next == UserInfo::states::pending) return;

  if (next == UserInfo::states::complete) {
    Audio().Play(AudioType::NEW_GAME);

    return;
  }

  user.currState = next;

  std::string dummy;
  if (next == UserInfo::states::entering_username) {
    dummy = "Enter Username";
  }

  inputInterface = new MessageInput(dummy, 20);

  if (next == UserInfo::states::entering_password) {
    inputInterface->ProtectPassword(true);
  }

  textbox.EnqueMessage(inputInterface);
  textbox.Open();
  Audio().Play(AudioType::CHIP_DESC);
}

void ConfigScene::onStart()
{
  Audio().Stream("resources/loops/config.ogg", false);
}

void ConfigScene::onLeave()
{
  Audio().StopStream();
}

void ConfigScene::onExit()
{
}

void ConfigScene::onEnter()
{
  Audio().StopStream();
}

void ConfigScene::onResume()
{
}

void ConfigScene::onEnd()
{
}
