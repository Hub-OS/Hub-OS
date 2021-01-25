#include <Swoosh/ActivityController.h>
#include <Segues/WhiteWashFade.h>
#include <Segues/BlackWashFade.h>

#include "bnConfigScene.h"
#include "bnRobotBackground.h"
#include "bnWebClientMananger.h"
#include "smart_enum.h"

// Columns. 
const constexpr int OPTIONS   = 0; // First column is top-level menu (option)
const constexpr int ACTIONS   = 1; // Second column is actions within that menu
const constexpr int BOUNDKEYS = 2; // Third column is used for bound keys

// Top-level menu items
SMART_ENUM(MenuItems,
  audio_bg,
  audio_sfx,
  shaders,
  keyboard_opts,
  gamepad_opts,
  account
)

ConfigScene::ConfigScene(swoosh::ActivityController &controller) : 
  textbox(sf::Vector2f(4,250)), 
  label(Font::Style::thin),
  Scene(controller)
{
  textbox.SetTextSpeed(4.0);
  isSelectingTopMenu = inGamepadList = inKeyboardList = false;

  // Draws the scrolling background
  bg = new RobotBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  endBtnAnimator = Animation("resources/scenes/config/end_btn.animation");
  endBtnAnimator.Load();

  audioAnimator = Animation("resources/scenes/config/audio.animation");
  audioAnimator.Load();

  // Audio() button
  audioBGM =  sf::Sprite(*LOAD_TEXTURE(AUDIO_ICO));
  audioBGM.setScale(2.f, 2.f);

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioBGM.setPosition(2*3, 2*140);

  audioSFX = audioBGM;
  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioSFX.setPosition(2 * 6 + 2 * 16, 2 * 140);

  // end button
  endBtn = sf::Sprite(*LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2*180, 2*10);

  for (std::string a : MenuItems_As_List) {
    uiData::ActionItemType type = uiData::ActionItemType::keyboard;

    if (a == "shaders") {
        type = uiData::ActionItemType::disabled;
        a = "SHADERS: OFF";
    }

    uiList[OPTIONS].push_back({ a, sf::Vector2f(), sf::Vector2f(), type });
  }

  configSettings = Input().GetConfigSettings();

  // For keyboard keys 
  for (auto& a : InputEvents::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::keyboard;

    uiList[ACTIONS].push_back({ a, sf::Vector2f(), sf::Vector2f(), type });

    if (!configSettings.IsOK()) {
      boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
    }
    else {
      std::string keyStr;

      if (Input().ConvertKeyToString(configSettings.GetPairedInput(a), keyStr)) {
        keyHash.insert(std::make_pair(configSettings.GetPairedInput(a), a));

        boundKeys.push_back({ keyStr,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
      }
    }
  }
  
  maxMenuSelectionIndex += static_cast<int>(MenuItems::size);

  // For gamepad keys
  for (auto a : InputEvents::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::gamepad;

    if (!configSettings.IsOK()) {
      boundGamepadButtons.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type, 255 });
    }
    else {
      auto gamepadCode = configSettings.GetPairedGamepadButton(a);
      gamepadHash.insert(std::make_pair(gamepadCode, a));

      if (gamepadCode != Gamepad::BAD_CODE) {
        std::string label = "BTN " + std::to_string((int)gamepadCode);

        switch (gamepadCode) {
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

        boundGamepadButtons.push_back({ label,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundGamepadButtons.push_back({ "NO KEY",  sf::Vector2f(), sf::Vector2f(), type });
      }
    }
  }

  leave = false;
  awaitingKey = false;
  gotoNextScene = true; // true when entering or leaving, prevents user from interacting with scene

  menuSelectionIndex = lastMenuSelectionIndex = 1; // select first item

  audioModeBGM = configSettings.GetMusicLevel();
  audioModeSFX = configSettings.GetSFXLevel();

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.SetFrame(audioModeBGM + 1, audioBGM);
  audioAnimator.SetFrame(audioModeSFX + 1, audioSFX);

  colIndex = 0; maxCols = 3; // [options] [actions] [key]

  setView(sf::Vector2u(480, 320));
}

void ConfigScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);
  bg->Update((float)elapsed);

  if (textbox.IsOpen()) {
    if (textbox.IsEndOfMessage()) {
      if (HasLeftButton()) {
        questionInterface->SelectYes();
      }
      else if (HasRightButton()) {
        questionInterface->SelectNo();
      }
      else if (HasCancelled()) {
        questionInterface->Cancel();
      }
      else if (HasConfirmed()) {
        questionInterface->ConfirmSelection();
      }
    }
    else if (HasConfirmed()) {
      if (!textbox.IsPlaying()) {
        questionInterface->Continue();
      }
      else {
        textbox.CompleteCurrentBlock();
      }
    }
  }

  if (!WEBCLIENT.IsLoggedIn()) {
    uiList[0][static_cast<size_t>(MenuItems::account)].label = "LOGIN";
  }
  else {
    uiList[0][static_cast<size_t>(MenuItems::account)].label = "LOGOUT " + WEBCLIENT.GetUserName();
  }

  if (HasConfirmed() && isSelectingTopMenu && !leave) {
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
        using effect = segue<BlackWashFade, milliseconds<300>>;
        getController().pop<effect>();
        leave = true;

        textbox.Close();
      };
      questionInterface = new Question("Overwrite your config settings?", onYes, onNo);
      textbox.EnqueMessage(sf::Sprite(), "", questionInterface);
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
  }
  
  if (!leave) {
    switch (currState) {
    case State::menu:
      UpdateMenuState(elapsed);
      break;
    case State::gamepad_select:
      UpdateGamepadState(elapsed);
      break;
    case State::login:
      UpdateLoginState(elapsed);
      break;
    }
  }

  // Make endBtn stick at the top of the form
  endBtn.setPosition(endBtn.getPosition().x, uiList[0][0].position.y - 60);

  if (isSelectingTopMenu) {
    endBtnAnimator.SetFrame(2, endBtn);
  }
  else {
    endBtnAnimator.SetFrame(1, endBtn);
  }
}

void ConfigScene::onDraw(sf::RenderTexture & surface)
{
  switch (currState) {
  case State::menu:
    DrawMenuState(surface);
    break;
  case State::gamepad_select:
    DrawGamepadState(surface);
    break;
  case State::login:
    DrawLoginState(surface);
    break;
  }

  surface.draw(textbox);
}

void ConfigScene::DrawMenuOptions(sf::RenderTarget& surface)
{
  for (int i = 0; i < 3; i++) {
    if (i > 0) {
      // Expand menu if the gamepad or keyboard option was selected
      if (!inGamepadList && !inKeyboardList) return;
    }

    int idx = 0;
    for (auto& ui : uiList[i]) {
      if (static_cast<MenuItems>(idx) == MenuItems::audio_bg) {
        audioBGM.setScale(ui.scale);
        audioBGM.setPosition(ui.position.x, ui.position.y);
        audioBGM.setColor(sf::Color(255, 0, 255, ui.alpha));
        surface.draw(audioBGM);

      }
      else if (static_cast<MenuItems>(idx) == MenuItems::audio_sfx) {
        audioSFX.setScale(ui.scale);
        audioSFX.setPosition(ui.position.x, ui.position.y);
        audioSFX.setColor(sf::Color(10, 165, 255, ui.alpha));
        surface.draw(audioSFX);
      }
      else {
        int offset = 0;
        label.SetString(ui.label);
        label.setScale(ui.scale);
        label.setPosition(2.f * (ui.position.x + offset), ui.position.y);

        if (ui.type == uiData::ActionItemType::keyboard) {
          label.SetColor(sf::Color(255, 165, 0, ui.alpha));
        }
        else if (ui.type == uiData::ActionItemType::disabled) {
          label.SetColor(sf::Color(255, 0, 0, ui.alpha));
        }
        else {
          label.SetColor(sf::Color(10, 165, 255, ui.alpha));
        }

        surface.draw(label);
      }
    }
  }
}

void ConfigScene::DrawMappedKeyMenu(std::vector<uiData>& container, sf::RenderTarget& surface)
{
  for (auto& ui : container) {
    label.SetString(ui.label);
    label.setScale(ui.scale);

    if (ui.type == uiData::ActionItemType::keyboard) {
      label.SetColor(sf::Color(255, 165, 0, ui.alpha));
    }
    else {
      label.SetColor(sf::Color(10, 165, 255, ui.alpha));
    }

    auto totalOffset = label.GetLocalBounds().width;
    label.setPosition((ui.position.x + 2.0f * (- totalOffset)), ui.position.y);


    surface.draw(label);
  }
}

void ConfigScene::DrawMenuState(sf::RenderTarget& surface)
{
  surface.draw(*bg);
  surface.draw(endBtn);
  surface.draw(audioBGM);
  surface.draw(audioSFX);

  // Draw options
  DrawMenuOptions(surface);

  if (inKeyboardList) {
    // Keyboard keys
    DrawMappedKeyMenu(boundKeys, surface);
  }

  if (inGamepadList) {
    // Gamepad keys
    DrawMappedKeyMenu(boundGamepadButtons, surface);
  }
}

void ConfigScene::UpdateMenuState(double elapsed)
{
  bool isInSubmenu = inKeyboardList || inGamepadList;

  if (HasCancelled() && !awaitingKey) {
    if (!isInSubmenu) {
      isSelectingTopMenu = true;
      colIndex = menuSelectionIndex = 0;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
    }
    else {
      if (inGamepadList) menuSelectionIndex = 4;
      if (inKeyboardList) menuSelectionIndex = 3;

      inGamepadList = inKeyboardList = false;
      isInSubmenu = false; // this flag affects the rest of this update step
      colIndex = 0;

      isSelectingTopMenu = false;
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
            if (iter->second == InputEvents::KEYS[menuSelectionIndex]) break;
            iter++;
          }

          if (iter != keyHash.end()) {
            keyHash.erase(iter);
          }

          keyHash.insert(std::make_pair(key, InputEvents::KEYS[menuSelectionIndex]));

          std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);
          boundKeys[menuSelectionIndex].label = boundKey;
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
          if (iter->second == InputEvents::KEYS[menuSelectionIndex]) break;
          iter++;
        }

        if (iter != gamepadHash.end()) {
          gamepadHash.erase(iter);
        }

        gamepadHash.insert(std::make_pair(gamepad, InputEvents::KEYS[menuSelectionIndex]));

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

        boundGamepadButtons[menuSelectionIndex].label = label;

        Audio().Play(AudioType::CHIP_DESC_CLOSE);

        awaitingKey = false;
      }
    }
  }
  else if (HasUpButton() && textbox.IsClosed()) {
    if (menuSelectionIndex == 0 && !isSelectingTopMenu) {
      isSelectingTopMenu = true;
      colIndex = 0;
    }
    else {
      menuSelectionIndex--;
      Audio().Play(AudioType::CHIP_SELECT);
    }
  }
  else if (HasDownButton() && textbox.IsClosed()) {
    if (menuSelectionIndex == 0 && isSelectingTopMenu) {
      isSelectingTopMenu = false;
    }
    else {
      menuSelectionIndex++;
      Audio().Play(AudioType::CHIP_SELECT);
    }
  }
  else if (HasLeftButton()) {
    // unused
  }
  else if (HasRightButton()) {
    // unused
  }
  else if (HasConfirmed() && !isSelectingTopMenu) {
    // bg audio
    if (menuSelectionIndex == static_cast<int>(MenuItems::audio_bg) && colIndex == 0) {
      audioModeBGM = (audioModeBGM + 1) % 4;
      Audio().SetStreamVolume(((audioModeBGM) / 3.0f) * 100.0f);
      audioAnimator.SetAnimation("DEFAULT");
      audioAnimator.SetFrame(audioModeBGM + 1, audioBGM);
      configSettings.SetMusicLevel(audioModeBGM);
    }
    else if (menuSelectionIndex == static_cast<int>(MenuItems::audio_sfx) && colIndex == 0) {
      audioModeSFX = (audioModeSFX + 1) % 4;
      Audio().SetChannelVolume(((audioModeSFX) / 3.0f) * 100.0f);
      audioAnimator.SetAnimation("DEFAULT");
      audioAnimator.SetFrame(audioModeSFX + 1, audioSFX);
      Audio().Play(AudioType::BUSTER_PEA);
      configSettings.SetSFXLevel(audioModeSFX);
    }
    else if (menuSelectionIndex == static_cast<int>(MenuItems::shaders) && colIndex == 0) {
      // TODO: Shader Toggle
      Audio().Play(AudioType::CHIP_ERROR);

    }
    else if (menuSelectionIndex == static_cast<int>(MenuItems::keyboard_opts) && colIndex == 0) {
      inKeyboardList = true;
      inGamepadList = false;
      colIndex = 1;
      menuSelectionIndex = 0; // move the row cursor to the top
      Audio().Play(AudioType::CHIP_SELECT);
    }
    else if (menuSelectionIndex == static_cast<int>(MenuItems::gamepad_opts) && colIndex == 0) {
      inGamepadList = true;
      inKeyboardList = false;
      colIndex = 1;
      menuSelectionIndex = 0; // move the row cursor to the top
      Audio().Play(AudioType::CHIP_SELECT);
    }
    else if (menuSelectionIndex == static_cast<int>(MenuItems::account) && colIndex == 0) {
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
          textbox.EnqueMessage(sf::Sprite(), "", questionInterface);
          textbox.Open();
          Audio().Play(AudioType::CHIP_DESC);
        }
      }
      else {
        // TODO: Show the login prompt
        inGamepadList = false;
        inKeyboardList = false;
        colIndex = 0;
        menuSelectionIndex = 0;
      }
    }
    else if (!awaitingKey) {
      awaitingKey = true;
      Audio().Play(AudioType::CHIP_DESC);
    }
  }


  if (isInSubmenu) {
    colIndex = std::max(1, colIndex);
  }
  else {
    colIndex = std::max(0, colIndex);
  }

  colIndex = std::min(colIndex, maxCols - 1 - 1); // we share the last colum: bound keys

  menuSelectionIndex = std::max(0, menuSelectionIndex);

  if (colIndex != BOUNDKEYS) {
    maxMenuSelectionIndex = static_cast<int>(uiList[colIndex].size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }
  else {
    maxMenuSelectionIndex = static_cast<int>(boundKeys.size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }

  if (lastMenuSelectionIndex != menuSelectionIndex) {
    Audio().Play(AudioType::CHIP_SELECT);
    lastMenuSelectionIndex = menuSelectionIndex;
  }

  lastMenuSelectionIndex = menuSelectionIndex;

  for (int j = 0; j < maxCols; j++) {
    std::vector<ConfigScene::uiData>* container = &boundKeys;

    if (inGamepadList) {
      container = &boundGamepadButtons;
    }

    if (j != BOUNDKEYS) {
      container = &uiList[j];
    }

    for (int i = 0; i < container->size(); i++) {
      auto w = 0.3f;
      auto diff = i - menuSelectionIndex;
      float scale = 1.0f - (w * abs(diff));
      auto colSpan = 45;
      auto lineSpan = 15;
      scale = std::max(scale, 0.8f);
      scale = std::min(scale, 1.0f);

      //scale = 1.0f;
      auto starty = 40;
      auto delta = 48.0f * static_cast<float>(elapsed);

      auto s = sf::Vector2f(2.f * scale, 2.f * scale);
      auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, (*container)[i].scale.x), swoosh::ease::interpolate(delta, s.y, (*container)[i].scale.y));

      // recalculate, unbounded this time
      scale = 1.0f - (w * abs(diff));
      scale = std::max(scale, 0.2f);
      scale = std::min(scale, 2.0f);

      auto limit = std::min(menuSelectionIndex, maxMenuSelectionIndex - 2); // stop the screen from rolling up when reaching this point of the list
      auto pos = sf::Vector2f(2.0f + 2.0f * (j * colSpan), 2.f * (starty + (i * lineSpan) - (limit * lineSpan)));
      auto lerp = sf::Vector2f(swoosh::ease::interpolate(delta, pos.x, (*container)[i].position.x), swoosh::ease::interpolate(delta, pos.y, (*container)[i].position.y));

      if (j != BOUNDKEYS) {
        (*container)[i].position = lerp;
      }
      else {
        // this effectively will right-align the bound keys
        (*container)[i].position.x = 238 * 2.f; //240px is the entire size of the screen
        (*container)[i].position.y = lerp.y;
        (*container)[i].scale = slerp;
      }

      if (j == 0 && colIndex == 0) {
        if (i != menuSelectionIndex) {
          (*container)[i].alpha = 150;

          if (awaitingKey) {
            (*container)[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = 255;
        }
      }
      else if (j > 0 && colIndex > 0) {
        if (i != menuSelectionIndex) {
          (*container)[i].alpha = 150;

          if (awaitingKey) {
            (*container)[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = 255;
        }
      }
      else {
        (*container)[i].alpha = 100;

        if (awaitingKey) {
          (*container)[i].alpha = 50;
        }
      }

      if (isSelectingTopMenu) {
        (*container)[i].alpha = 100;
      }
      // end for-loop
    }
  }
}

void ConfigScene::DrawGamepadState(sf::RenderTarget& surface)
{
}

void ConfigScene::UpdateGamepadState(double elapsed)
{
}

void ConfigScene::DrawLoginState(sf::RenderTarget& surface)
{
}

void ConfigScene::UpdateLoginState(double elapsed)
{
}

const bool ConfigScene::HasConfirmed() const
{
  return Input().GetAnyKey() == sf::Keyboard::Return;
}

const bool ConfigScene::HasCancelled() const
{
  return (Input().GetAnyKey() == sf::Keyboard::BackSpace || Input().GetAnyKey() == sf::Keyboard::Escape);
}

const bool ConfigScene::HasUpButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Up;
}

const bool ConfigScene::HasDownButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Down;
}

const bool ConfigScene::HasLeftButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Left;
}

const bool ConfigScene::HasRightButton() const
{
  return Input().GetAnyKey() == sf::Keyboard::Right;
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
