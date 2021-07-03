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
  const std::function<void(TextItem&)>& callback,
  const std::function<void(TextItem&)>& secondaryCallback
) :
  MenuItem(
    [this, callback] { callback(*this); },
    [this, secondaryCallback] { secondaryCallback(*this); }
  ),
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

ConfigScene::LoginItem::LoginItem(const std::function<void()>& callback) : TextItem("Login", [callback](auto&) { callback(); }) {}

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
  const std::function<void(BindingItem&)>& callback,
  const std::function<void(BindingItem&)>& secondaryCallback
) :
  TextItem(
    inputName,
    [this, callback](auto&) { callback(*this); },
    [this, secondaryCallback](auto&) { secondaryCallback(*this); }
  ),
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
  MenuItem(createCallback(callback), createSecondaryCallback(callback)),
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
  // raise volume
  return [this, callback] {
    volumeLevel = (volumeLevel + 1) % 4;
    animator.SetFrame(volumeLevel + 1, icon.getSprite());
    callback(volumeLevel);
  };
}

std::function<void()> ConfigScene::VolumeItem::createSecondaryCallback(const std::function<void(int)>& callback) {
  // lower volume
  return [this, callback] {
    volumeLevel = volumeLevel - 1;

    if (volumeLevel < 0) {
      volumeLevel = 3;
    }

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
  isSelectingTopMenu = false;

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
  // BGM
  primaryMenu.push_back(std::make_unique<VolumeItem>(
    sf::Color(255, 0, 255),
    configSettings.GetMusicLevel(),
    [this](int volumeLevel) { UpdateBgmVolume(volumeLevel); })
  );
  // SFX
  primaryMenu.push_back(std::make_unique<VolumeItem>(
    sf::Color(10, 165, 255),
    configSettings.GetSFXLevel(),
    [this](int volumeLevel) { UpdateSfxVolume(volumeLevel); })
  );
  // Shaders
  auto shadersItem = std::make_unique<TextItem>("SHADERS: ON", [this](auto&) { ToggleShaders(); });
  shadersItem->SetColor(DISABLED_TEXT_COLOR);
  primaryMenu.push_back(std::move(shadersItem));
  // Keyboard
  primaryMenu.push_back(std::make_unique<TextItem>(
    "MY KEYBOARD",
    [this](auto&) { ShowKeyboardMenu(); })
  );
  // Gamepad
  primaryMenu.push_back(std::make_unique<TextItem>(
    "MY GAMEPAD",
    [this](auto&) { ShowGamepadMenu(); })
  );
  // Login
  primaryMenu.push_back(std::make_unique<LoginItem>(
    [this] { ToggleLogin(); })
  );

  configSettings = Input().GetConfigSettings();

  // For keyboard keys 
  auto keyCallback = [this](BindingItem& item) { AwaitKeyBinding(item); };
  auto keySecondaryCallback = [this](BindingItem& item) { UnsetKeyBinding(item); };

  for (auto eventName : InputEvents::KEYS) {

    if (configSettings.IsOK()) {
      std::optional<std::reference_wrapper<std::string>> value;
      std::string keyStr;

      if (Input().ConvertKeyToString(configSettings.GetPairedInput(eventName), keyStr)) {
        keyHash.insert(std::make_pair(configSettings.GetPairedInput(eventName), eventName));
        value = keyStr;
      }

      keyboardMenu.push_back(std::make_unique<BindingItem>(eventName, value, keyCallback, keySecondaryCallback));
    }
  }

  // Adjusting gamepad index (abusing BindingItem for alignment)
  gamepadIndex = configSettings.GetGamepadIndex();
  auto gamepadIndexString = std::to_string(gamepadIndex);
  gamepadMenu.push_back(std::make_unique<BindingItem>(
    "Gamepad Index",
    gamepadIndexString,
    [this](BindingItem& item) { IncrementGamepadIndex(item); },
    [this](BindingItem& item) { DecrementGamepadIndex(item); }
  ));

  // For gamepad keys
  auto gamepadCallback = [this](BindingItem& item) { AwaitGamepadBinding(item); };
  auto gamepadSecondaryCallback = [this](BindingItem& item) { UnsetGamepadBinding(item); };

  for (auto eventName : InputEvents::KEYS) {
    std::optional<std::reference_wrapper<std::string>> value;
    std::string valueString;

    if (configSettings.IsOK()) {
      auto gamepadCode = configSettings.GetPairedGamepadButton(eventName);
      gamepadHash.insert(std::make_pair(gamepadCode, eventName));

      if (gamepadCode != Gamepad::BAD_CODE) {
        valueString = "BTN " + std::to_string((int)gamepadCode);

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

    gamepadMenu.push_back(std::make_unique<BindingItem>(
      eventName,
      value,
      gamepadCallback,
      gamepadSecondaryCallback
      ));
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
  activeSubmenu = keyboardMenu;
  Audio().Play(AudioType::CHIP_SELECT);
}

void ConfigScene::ShowGamepadMenu() {
  activeSubmenu = gamepadMenu;
  Audio().Play(AudioType::CHIP_SELECT);
}

void ConfigScene::ToggleLogin() {
  if (WEBCLIENT.IsLoggedIn()) {
    if (textbox.IsClosed()) {
      auto onYes = [this]() {
        Logger::Log("SendLogoutCommand");
        WEBCLIENT.SendLogoutCommand();
      };

      auto onNo = [this]() {
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
  pendingKeyBinding = item;
}

void ConfigScene::UnsetKeyBinding(BindingItem& item) {
  auto& eventName = item.GetString();
  auto iter = keyHash.begin();

  while (iter != keyHash.end()) {
    if (iter->second == eventName) break;
    iter++;
  }

  if (iter != keyHash.end()) {
    keyHash.erase(iter);
  }

  item.SetValue({});
}

void ConfigScene::AwaitGamepadBinding(BindingItem& item) {
  pendingGamepadBinding = item;

  // disable gamepad so we can escape binding in case the gamepad is not working or not plugged in
  gamepadWasActive = Input().IsUsingGamepadControls();
  Input().UseGamepadControls(false);
}

void ConfigScene::UnsetGamepadBinding(BindingItem& item) {
  auto& eventName = item.GetString();
  auto iter = gamepadHash.begin();

  while (iter != gamepadHash.end()) {
    if (iter->second == eventName) break;
    iter++;
  }

  if (iter != gamepadHash.end()) {
    gamepadHash.erase(iter);
  }
}

void ConfigScene::IncrementGamepadIndex(BindingItem& item) {
  if (++gamepadIndex >= Input().GetGamepadCount()) {
    gamepadIndex = 0;
  }

  // set gamepad now to allow binding it's input
  Input().UseGamepad(gamepadIndex);

  auto indexString = std::to_string(gamepadIndex);
  item.SetValue(indexString);
}

void ConfigScene::DecrementGamepadIndex(BindingItem& item) {
  if (--gamepadIndex < 0) {
    gamepadIndex = Input().GetGamepadCount() > 0 ? int(Input().GetGamepadCount()) - 1 : 0;
  }

  // set gamepad now to allow binding it's input
  Input().UseGamepad(gamepadIndex);

  auto indexString = std::to_string(gamepadIndex);
  item.SetValue(indexString);
}

bool ConfigScene::IsInSubmenu() {
  return activeSubmenu.has_value();
}

ConfigScene::Menu& ConfigScene::GetActiveMenu() {
  return IsInSubmenu() ? activeSubmenu->get() : primaryMenu;
}

int& ConfigScene::GetActiveIndex() {
  return IsInSubmenu() ? submenuIndex : primaryIndex;
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

  bool hasConfirmed = Input().Has(InputEvents::pressed_confirm);
  bool hasSecondary = Input().Has(InputEvents::pressed_option);

  if (hasConfirmed && isSelectingTopMenu && !leave) {
    if (textbox.IsClosed()) {
      auto onYes = [this]() {
        // backup keyboard hash in case the current hash is invalid
        auto oldKeyboardHash = configSettings.GetKeyboardHash();

        // Save before leaving
        configSettings.SetKeyboardHash(keyHash);

        if (!configSettings.TestKeyboard()) {
          // revert
          configSettings.SetKeyboardHash(oldKeyboardHash);

          // block exit with warning
          messageInterface = new Message("Keyboard has overlapping or unset bindings.");
          textbox.EnqueMessage(sf::Sprite(), "", messageInterface);
          Audio().Play(AudioType::CHIP_ERROR, AudioPriority::high);
          return;
        }

        configSettings.SetGamepadIndex(gamepadIndex);
        configSettings.SetGamepadHash(gamepadHash);

        ConfigWriter writer(configSettings);
        writer.Write("config.ini");
        ConfigReader reader("config.ini");
        Input().SupportConfigSettings(reader);

        // transition to the next screen
        using namespace swoosh::types;
        using effect = segue<WhiteWashFade, milliseconds<300>>;
        getController().pop<effect>();

        Audio().Play(AudioType::NEW_GAME);
        leave = true;
      };

      auto onNo = [this]() {
        // Use config stored gamepad index in case we were messing with another controller here
        Input().UseGamepad(configSettings.GetGamepadIndex());

        // Just close and leave
        using namespace swoosh::types;
        using effect = segue<WhiteWashFade, milliseconds<300>>;
        getController().pop<effect>();
        leave = true;
      };
      questionInterface = new Question("Overwrite your config settings?", onYes, onNo);
      textbox.EnqueMessage(sf::Sprite(), "", questionInterface);
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
  }

  if (!leave) {
    bool hasCanceled = Input().Has(InputEvents::pressed_cancel);

    bool hasUp = Input().Has(InputEvents::pressed_ui_up);
    bool hasDown = Input().Has(InputEvents::pressed_ui_down);
    bool hasLeft = Input().Has(InputEvents::pressed_ui_left);
    bool hasRight = Input().Has(InputEvents::pressed_ui_right);

    if (textbox.IsOpen()) {
      if (messageInterface) {
        if (hasConfirmed) {
          // continue the conversation if the text is complete
          if (textbox.IsEndOfMessage()) {
            textbox.DequeMessage();
            messageInterface = nullptr;
          }
          else if (textbox.IsEndOfBlock()) {
            textbox.ShowNextLines();
          }
          else {
            // double tapping talk will complete the block
            textbox.CompleteCurrentBlock();
          }
        }
      }
      else if (questionInterface) {
        if (textbox.IsEndOfMessage()) {
          if (hasLeft) {
            questionInterface->SelectYes();
          }
          else if (hasRight) {
            questionInterface->SelectNo();
          }
          else if (hasCanceled) {
            questionInterface->Cancel();
            questionInterface = nullptr;
          }
          else if (hasConfirmed) {
            questionInterface->ConfirmSelection();
            questionInterface = nullptr;
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

      if (textbox.IsEndOfMessage() && !textbox.HasMessage()) {
        textbox.Close();
      }
    }
    else if (pendingKeyBinding || pendingGamepadBinding) {
      if (pendingKeyBinding) {
        auto key = Input().GetAnyKey();

        if (key != sf::Keyboard::Unknown) {
          std::string boundKey = "";

          if (Input().ConvertKeyToString(key, boundKey)) {
            auto& menuItem = pendingKeyBinding->get();
            auto& eventName = menuItem.GetString();

            UnsetKeyBinding(menuItem);

            keyHash.insert(std::make_pair(key, eventName));

            std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);

            menuItem.SetValue(boundKey);

            Audio().Play(AudioType::CHIP_DESC_CLOSE);

            pendingKeyBinding = {};
          }
        }
      }

      if (hasCanceled) {
        pendingGamepadBinding = {};

        // re-enable gamepad if it was on
        Input().UseGamepadControls(gamepadWasActive);
      } if (pendingGamepadBinding) {
        // GAMEPAD
        auto gamepad = Input().GetAnyGamepadButton();

        if (gamepad != (Gamepad)-1) {
          auto& menuItem = pendingGamepadBinding->get();
          auto& eventName = menuItem.GetString();

          UnsetGamepadBinding(menuItem);

          gamepadHash.insert(std::make_pair(gamepad, eventName));

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

          menuItem.SetValue(label);

          Audio().Play(AudioType::CHIP_DESC_CLOSE);

          pendingGamepadBinding = {};
        }
      }
    }
    else if (hasCanceled) {
      if (!IsInSubmenu()) {
        Audio().Play(AudioType::CHIP_DESC_CLOSE);
      }
      else {
        activeSubmenu = {};
        submenuIndex = 0;

        Audio().Play(AudioType::CHIP_DESC_CLOSE);
      }
    }
    else if (hasUp && textbox.IsClosed()) {
      auto& activeIndex = GetActiveIndex();

      if (primaryIndex == 0 && !isSelectingTopMenu) {
        isSelectingTopMenu = true;
      }
      else {
        activeIndex--;
      }

      Audio().Play(AudioType::CHIP_SELECT);
    }
    else if (hasDown && textbox.IsClosed()) {
      auto& activeIndex = GetActiveIndex();

      if (isSelectingTopMenu) {
        isSelectingTopMenu = false;
      }
      else {
        activeIndex++;
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
      auto& activeMenu = GetActiveMenu();
      activeMenu[GetActiveIndex()]->Select();
    }
    else if (hasSecondary && !isSelectingTopMenu) {
      auto& activeMenu = GetActiveMenu();
      activeMenu[GetActiveIndex()]->SecondarySelect();
    }
  }

  primaryIndex = std::clamp(primaryIndex, 0, int(primaryMenu.size()) - 1);


  UpdateMenu(primaryMenu, !IsInSubmenu(), primaryIndex, 0, float(elapsed));

  // display submenu, currently just keyboard/gamepad binding
  if (IsInSubmenu()) {
    auto& submenu = activeSubmenu->get();
    submenuIndex = std::clamp(submenuIndex, 0, int(submenu.size()) - 1);

    UpdateMenu(submenu, true, submenuIndex, SUBMENU_SPAN, float(elapsed));
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
  auto menuSize = GetActiveMenu().size();
  auto selectionIndex = GetActiveIndex();

  auto scrollEnd = std::max(0.0f, float(menuSize + 1) * LINE_SPAN - getView().getSize().y / 2.0f + MENU_START_Y);
  auto scrollIncrement = scrollEnd / float(menuSize);
  auto newScrollOffset = -float(selectionIndex) * scrollIncrement;
  scrollOffset = swoosh::ease::interpolate(float(elapsed) * SCROLL_INTERPOLATION_MULTIPLIER, scrollOffset, newScrollOffset);
}

void ConfigScene::UpdateMenu(Menu& menu, bool menuHasFocus, int selectionIndex, float offsetX, float elapsed) {
  for (int index = 0; index < menu.size(); index++) {
    auto& menuItem = *menu[index];
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

    bool awaitingBinding = pendingKeyBinding || pendingGamepadBinding;

    if (menuHasFocus) {
      if (index == selectionIndex) {
        menuItem.SetAlpha(255);
      }
      else {
        menuItem.SetAlpha(awaitingBinding ? 50 : 150);
      }
    }
    else {
      menuItem.SetAlpha(awaitingBinding ? 50 : 100);
    }

    menuItem.Update();
  }
}

void ConfigScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(*bg);

  // scrolling the view
  auto states = sf::RenderStates::Default;
  states.transform.translate(0, 2.0f * scrollOffset);

  surface.draw(endBtn, states);

  for (auto& menuItem : primaryMenu) {
    surface.draw(*menuItem, states);
  }

  if (activeSubmenu) {
    auto& submenu = activeSubmenu->get();
    for (auto& menuItem : submenu) {
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
