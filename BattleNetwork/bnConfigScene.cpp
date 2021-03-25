#include "bnConfigScene.h"
#include "bnWebClientMananger.h"
#include "Segues/WhiteWashFade.h"
#include "bnRobotBackground.h"

#undef GetUserName

// Columns. 
const constexpr int OPTIONS = 0; // First column is top-level menu (option)
const constexpr int ACTIONS = 1; // Second column is actions within that menu
const constexpr int BOUNDKEYS = 2; // Third column is used for bound keys

ConfigScene::ConfigScene(swoosh::ActivityController& controller) :
  textbox(sf::Vector2f(4, 250)), 
  Scene(controller)
{
  textbox.SetTextSpeed(4.0);
  isSelectingTopMenu = inGamepadList = inKeyboardList = false;

  // Draws the scrolling background
  bg = new RobotBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  endBtnAnimator = Animation("resources/ui/config/end_btn.animation");
  endBtnAnimator.Load();

  audioAnimator = Animation("resources/ui/config/audio.animation");
  audioAnimator.Load();

  // audio button
  audioBGM.setTexture(Textures().LoadTextureFromFile("resources/ui/config/audio.png"));
  audioBGM.setScale(2.f, 2.f);

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM.getSprite());
  audioBGM.setPosition(2 * 3, 2 * 140);

  audioSFX.setTexture(Textures().LoadTextureFromFile("resources/ui/config/audio.png"));
  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioSFX.getSprite());
  audioSFX.setPosition(2 * 6 + 2 * 16, 2 * 140);

  // end button
  endBtn = sf::Sprite(*LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2 * 180, 2 * 10);

  // ui sprite maps
  // ascii 58 - 96
  std::list<std::string> actions;

  actions.push_back("AUDIO_BGM");
  actions.push_back("AUDIO_SFX");
  actions.push_back("SHADERS: ON");
  actions.push_back("MY KEYBOARD");
  actions.push_back("MY GAMEPAD");
  actions.push_back("LOGIN");

  for (auto a : actions) {
    uiData::ActionItemType type = uiData::ActionItemType::KEYBOARD;

    if (a == "SHADERS: ON") {
      type = uiData::ActionItemType::DISABLED;
    }

    uiList[OPTIONS].push_back(uiData{ a, sf::Vector2f(), sf::Vector2f(), type });
  }

  actions.clear();

  configSettings = Input().GetConfigSettings();

  // For keyboard keys 
  for (auto a : InputEvents::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::KEYBOARD;

    uiList[ACTIONS].push_back(uiData{ a, sf::Vector2f(), sf::Vector2f(), type });

    if (!configSettings.IsOK()) {
      boundKeys.push_back(uiData{ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
    }
    else {
      std::string keyStr;

      if (Input().ConvertKeyToString(configSettings.GetPairedInput(a), keyStr)) {
        keyHash.insert(std::make_pair(configSettings.GetPairedInput(a), a));

        boundKeys.push_back(uiData{ keyStr,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundKeys.push_back(uiData{ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
      }
    }
  }

  maxMenuSelectionIndex += (int)actions.size();

  // For gamepad keys

  for (auto a : InputEvents::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::GAMEPAD;

    if (!configSettings.IsOK()) {
      boundGamepadButtons.push_back(uiData{ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
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

        boundGamepadButtons.push_back(uiData{ label,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundGamepadButtons.push_back(uiData{ "NO KEY",  sf::Vector2f(), sf::Vector2f(), type });
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
  audioAnimator.SetFrame(audioModeBGM + 1, audioBGM.getSprite());
  audioAnimator.SetFrame(audioModeSFX + 1, audioSFX.getSprite());

  colIndex = 0; maxCols = 3; // [options] [actions] [key]

  setView(sf::Vector2u(480, 320));
}

ConfigScene::~ConfigScene() { }

void ConfigScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);
  bg->Update((float)elapsed);

  if (!WEBCLIENT.IsLoggedIn()) {
    uiList[0][uiList[0].size() - 1].label = "LOGIN";
  }
  else {
    uiList[0][uiList[0].size() - 1].label = "LOGOUT " + WEBCLIENT.GetUserName();
  }

  bool hasConfirmed = (Input().IsConfigFileValid() ? Input().Has(InputEvents::pressed_confirm) : false) || Input().GetAnyKey() == sf::Keyboard::Enter;
  bool isInSubmenu = inKeyboardList || inGamepadList;

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
    else if (hasCanceled && !awaitingKey) {
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
      if (hasCanceled) {
        awaitingKey = false;
      }
      else if (inKeyboardList) {
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
    else if (hasUp && textbox.IsClosed()) {
      if (menuSelectionIndex == 0 && !isSelectingTopMenu) {
        isSelectingTopMenu = true;
        colIndex = 0;
      }
      else {
        menuSelectionIndex--;
        Audio().Play(AudioType::CHIP_SELECT);

      }
    }
    else if (hasDown && textbox.IsClosed()) {
      if (menuSelectionIndex == 0 && isSelectingTopMenu) {
        isSelectingTopMenu = false;
      }
      else {
        menuSelectionIndex++;
        Audio().Play(AudioType::CHIP_SELECT);
      }
    }
    else if (hasLeft) {
      // unused
    }
    else if (hasRight) {
      // unused
    }
    else if (hasConfirmed && !isSelectingTopMenu) {
      // bg audio
      if (menuSelectionIndex == 0 && colIndex == 0) {
        audioModeBGM = (audioModeBGM + 1) % 4;
        Audio().SetStreamVolume(((audioModeBGM) / 3.0f) * 100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeBGM + 1, audioBGM.getSprite());
        configSettings.SetMusicLevel(audioModeBGM);
      }
      else if (menuSelectionIndex == 1 && colIndex == 0) {
        audioModeSFX = (audioModeSFX + 1) % 4;
        Audio().SetChannelVolume(((audioModeSFX) / 3.0f) * 100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeSFX + 1, audioSFX.getSprite());
        Audio().Play(AudioType::BUSTER_PEA);
        configSettings.SetSFXLevel(audioModeSFX);
      }
      else if (menuSelectionIndex == 2 && colIndex == 0) {
        // TODO: Shader Toggle
        Audio().Play(AudioType::CHIP_ERROR);

      }
      else if (menuSelectionIndex == 3 && colIndex == 0) {
        inKeyboardList = true;
        inGamepadList = false;
        colIndex = 1;
        menuSelectionIndex = 0; // move the row cursor to the top
        Audio().Play(AudioType::CHIP_SELECT);
      }
      else if (menuSelectionIndex == 4 && colIndex == 0) {
        inGamepadList = true;
        inKeyboardList = false;
        colIndex = 1;
        menuSelectionIndex = 0; // move the row cursor to the top
        Audio().Play(AudioType::CHIP_SELECT);
      }
      else if (menuSelectionIndex == 5 && colIndex == 0) {

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
          inLoginMenu = true;
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
    maxMenuSelectionIndex = int(uiList[colIndex].size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }
  else {
    maxMenuSelectionIndex = int(boundKeys.size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }

  if (lastMenuSelectionIndex != menuSelectionIndex) {
    lastMenuSelectionIndex = menuSelectionIndex;
  }

  lastMenuSelectionIndex = menuSelectionIndex;

  for (int j = 0; j < maxCols; j++) {
    std::vector<uiData>* container = &boundKeys;

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
      auto delta = 48.0f * float(elapsed);

      auto s = sf::Vector2f(2.f * scale, 2.f * scale);
      auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, (*container)[i].scale.x), swoosh::ease::interpolate(delta, s.y, (*container)[i].scale.y));
      (*container)[i].scale = slerp;

      // recalculate, unbounded this time
      scale = 1.0f - (w * abs(diff));
      scale = std::max(scale, 0.2f);
      scale = std::min(scale, 2.0f);

      s = sf::Vector2f(2.f * scale, 2.f * scale);

      auto limit = std::min(menuSelectionIndex, maxMenuSelectionIndex - 1); // stop the screen from rolling up when reaching this point of the list
      auto pos = sf::Vector2f(/*2.f * (5 + (3 * s.x))*/2.0f + 2.0f * (j * colSpan), 2.f * (starty + (i * lineSpan) - (limit * 10)));
      auto lerp = sf::Vector2f(swoosh::ease::interpolate(delta, pos.x, (*container)[i].position.x), swoosh::ease::interpolate(delta, pos.y, (*container)[i].position.y));

      if (j != BOUNDKEYS) {
        (*container)[i].position = lerp;
      }
      else {
        // this effectively will right-align the bound keys
        (*container)[i].position.x = 238 * 2.f; //240px is the entire size of the screen
        (*container)[i].position.y = lerp.y;
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

  // Make endBtn stick at the top of the screen
  endBtn.setPosition(endBtn.getPosition().x, uiList[0][0].position.y - 60);

  if (isSelectingTopMenu) {
    endBtnAnimator.SetFrame(2, endBtn);
  }
  else {
    endBtnAnimator.SetFrame(1, endBtn);
  }
}

void ConfigScene::onDraw(sf::RenderTexture& surface)
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

  surface.draw(textbox);
}

void ConfigScene::DrawMenuOptions(sf::RenderTexture& surface)
{
  for (int i = 0; i < 3; i++) {
    if (i > 0) {
      // Expand menu if the gamepad or keyboard option was selected
      if (!inGamepadList && !inKeyboardList) return;
    }

    for (auto ui : uiList[i]) {
      if (ui.label == "AUDIO_BGM") {
        audioBGM.setScale(ui.scale);
        audioBGM.setPosition(ui.position.x, ui.position.y);
        audioBGM.setColor(sf::Color(255, 0, 255, ui.alpha));
      }
      else if (ui.label == "AUDIO_SFX") {
        audioSFX.setScale(ui.scale);
        audioSFX.setPosition(ui.position.x, ui.position.y);
        audioSFX.setColor(sf::Color(10, 165, 255, ui.alpha));

      }
      else {
        Text label{ Font::Style::wide };
        label.setScale(ui.scale);
        label.setPosition(2.f * (ui.position.x), ui.position.y);
        label.SetString(ui.label);

        if (ui.type == uiData::ActionItemType::KEYBOARD) {
          label.SetColor(sf::Color(255, 165, 0, ui.alpha));
        }
        else if (ui.type == uiData::ActionItemType::DISABLED) {
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

void ConfigScene::DrawMappedKeyMenu(std::vector<uiData>& container, sf::RenderTexture& surface)
{
  for (auto ui : container) {
    Text label{ Font::Style::wide };
    label.setScale(ui.scale);
    label.setPosition(2.f * (ui.position.x), ui.position.y);
    label.SetString(ui.label);

    if (ui.type == uiData::ActionItemType::KEYBOARD) {
      label.SetColor(sf::Color(255, 165, 0, ui.alpha));
    }
    else if (ui.type == uiData::ActionItemType::DISABLED) {
      label.SetColor(sf::Color(255, 0, 0, ui.alpha));
    }
    else {
      label.SetColor(sf::Color(10, 165, 255, ui.alpha));
    }

    label.setScale(ui.scale);
    label.setOrigin(sf::Vector2f{ label.GetLocalBounds().width,0.f });
    label.setPosition(ui.position.x + 2.0f, ui.position.y);

    if (ui.type == uiData::ActionItemType::KEYBOARD) {
      label.SetColor(sf::Color(255, 165, 0, ui.alpha));
    }
    else {
      label.SetColor(sf::Color(10, 165, 255, ui.alpha));
    }

    surface.draw(label);
  }
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
