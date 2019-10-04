#include "bnConfigScene.h"
#include <Swoosh\ActivityController.h>
#include "Segues\ZoomFadeIn.h"

const constexpr int OPTIONS   = 0;
const constexpr int ACTIONS   = 1;
const constexpr int BOUNDKEYS = 2;

ConfigScene::ConfigScene(swoosh::ActivityController &controller) : swoosh::Activity(&controller)
{
  isSelectingTopMenu = false;

  // Draws the scrolling background
  bg = new LanBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  uiAnimator = Animation("resources/fonts/fonts.animation");
  uiAnimator.Load();

  endBtnAnimator = Animation("resources/backgrounds/config/end_btn.animation");
  endBtnAnimator.Load();

  audioAnimator = Animation("resources/backgrounds/config/audio.animation");
  audioAnimator.Load();

  auto sprite = sf::Sprite(LOAD_TEXTURE(FONT));
  sprite.setScale(2.f, 2.f);

  uiSprite = sprite;

  // audio button
  audioBGM =  sf::Sprite(LOAD_TEXTURE(AUDIO_ICO));
  audioBGM.setScale(2.f, 2.f);

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioBGM.setPosition(2*3, 2*140);

  audioSFX = audioBGM;
  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioSFX.setPosition(2 * 6 + 2 * 16, 2 * 140);

  // end button
  endBtn = sf::Sprite(LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2*180, 2*10);

  // ui sprite maps
  // ascii 58 - 96
  std::list<std::string> actions;

  actions.push_back("AUDIO_BGM");
  actions.push_back("AUDIO_SFX");
  actions.push_back("SHADERS: ON");

  for (auto a : actions) {
    uiList[OPTIONS].push_back({a, sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::BATTLE });
  }

  actions.clear();

  Logger::Log("input manager request");
  configSettings = INPUT.GetConfigSettings();

  // For input keys 

  for (auto a : EventTypes::KEYS) {
    uiList[ACTIONS].push_back({ a, sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::BATTLE });

    if (false /*!configSettings.IsOK()*/) {
      boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::BATTLE });
    }
    else {
      std::string keyStr;

      if (INPUT.ConvertKeyToString(configSettings.GetPairedInput(a), keyStr)) {
        boundKeys.push_back({ keyStr,  sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::BATTLE });
      }
      else {
        boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::BATTLE });
      }
    }
  }
  
  menuDivideIndex = maxMenuSelectionIndex = (int)actions.size();

  maxMenuSelectionIndex += (int)actions.size();

  // For menu keys

  for (auto a : actions) {
    uiList[ACTIONS].push_back({ a, sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::MENUS });
    boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), uiData::ActionItemType::MENUS });
  }

  leave = false;
  awaitingKey = false;
  gotoNextScene = true; // true when entering or leaving, prevents user from interacting with scene

  menuSelectionIndex = lastMenuSelectionIndex = 0;

  audioModeBGM = 3; // full
  audioModeSFX = 3;
  colIndex = 0; maxCols = 3; // [options] [actions] [key]
}

void ConfigScene::onUpdate(double elapsed)
{
  bg->Update((float)elapsed);

  if (INPUT.Has(EventTypes::PRESSED_CONFIRM) && isSelectingTopMenu && !leave) {
    if (!awaitingKey) {
      using namespace swoosh::intent;
      using effect = segue<ZoomFadeIn, swoosh::intent::milli<500>>;
      getController().queuePop<effect>();
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      leave = true;

      ConfigWriter writer(configSettings);
      writer.Write("options.ini");
    }
  }

  if (INPUT.Has(EventTypes::HELD_CANCEL)) {
    isSelectingTopMenu = true;
    colIndex = menuSelectionIndex = 0;
  }

  if (!leave) {
    if (awaitingKey) {
      auto key = INPUT.GetAnyKey();

      if (key != sf::Keyboard::Unknown) {
        std::string boundKey = "";

        if (INPUT.ConvertKeyToString(key, boundKey)) {
          std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);
          boundKeys[menuSelectionIndex].label = boundKey;
          AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

          awaitingKey = false;
        }
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
      if (menuSelectionIndex == 0 && !isSelectingTopMenu) {
        isSelectingTopMenu = true;
        colIndex = 0;
      }
      else {
        menuSelectionIndex--;
      }
    } else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
      if (menuSelectionIndex == 0 && isSelectingTopMenu) {
        isSelectingTopMenu = false;
      }
      else {
        menuSelectionIndex++;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
      if (!isSelectingTopMenu) {
        colIndex--;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
      if (!isSelectingTopMenu) {
        colIndex++;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
      // bg audio
      if (menuSelectionIndex == 0 && colIndex == 0) {
        audioModeBGM = (audioModeBGM+1) % 4;
        AUDIO.SetStreamVolume (((audioModeBGM)/3.0f)*100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeBGM + 1, audioBGM);


      }
      else if (menuSelectionIndex == 1 && colIndex == 0) {
        audioModeSFX = (audioModeSFX + 1) % 4;
        AUDIO.SetChannelVolume(((audioModeSFX) / 3.0f)*100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeSFX + 1, audioSFX);
        AUDIO.Play(AudioType::BUSTER_PEA);
      }
      else if(!awaitingKey) {
        awaitingKey = true;
        AUDIO.Play(AudioType::CHIP_DESC);
      }
    }
  }

  colIndex = std::max(0, colIndex);
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
    AUDIO.Play(AudioType::CHIP_SELECT);

    lastMenuSelectionIndex = menuSelectionIndex;
  }

  lastMenuSelectionIndex = menuSelectionIndex;

  for (int j = 0; j < maxCols; j++) {
    
    std::vector<uiData>* container = &boundKeys;
    
    if(j != BOUNDKEYS) container = &uiList[j];

    for (int i = 0; i < container->size(); i++) {
      auto w = 0.3f;
      auto diff = i - menuSelectionIndex;
      float scale = 1.0f - (w*abs(diff));
      auto colSpan = 45;
      auto lineSpan = 15;
      scale = std::max(scale, 0.8f);
      scale = std::min(scale, 1.0f);

      //scale = 1.0f;
      auto starty = 40;
      auto delta = 48.0f *float(elapsed);

      auto s = sf::Vector2f(2.f*scale, 2.f*scale);
      auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, (*container)[i].scale.x), swoosh::ease::interpolate(delta, s.y, (*container)[i].scale.y));
      (*container)[i].scale = slerp;

      // recalculate, unbounded this time
      scale = 1.0f - (w*abs(diff));
      scale = std::max(scale, 0.3f);
      scale = std::min(scale, 1.5f);

      s = sf::Vector2f(2.f*scale, 2.f*scale);

      auto limit = std::min(menuSelectionIndex, maxMenuSelectionIndex - 2); // stop the screen from rolling up when reaching this point of the list
      auto pos = sf::Vector2f(/*2.f * (5 + (3 * s.x))*/2.0f+2.0f*(j*colSpan), 2.f*(starty + (i * lineSpan) - (limit * 10)));
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
          (*container)[i].alpha = boundKeys[i].alpha = 100;

          if (awaitingKey) {
            (*container)[i].alpha = boundKeys[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = boundKeys[i].alpha = 255;
        }
      }
      else if(j > 0 && colIndex > 0) {
        if (i != menuSelectionIndex) {
          (*container)[i].alpha = boundKeys[i].alpha = 100;

          if (awaitingKey) {
            (*container)[i].alpha = boundKeys[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = boundKeys[i].alpha = 255;
        }
      }
      else {
        (*container)[i].alpha = boundKeys[i].alpha = 100;

        if (awaitingKey) {
          (*container)[i].alpha = boundKeys[i].alpha = 50;
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

void ConfigScene::onDraw(sf::RenderTexture & surface)
{
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(bg);

 
  ENGINE.Draw(endBtn);

  for (auto col : uiList) {

    for (auto ui : col) {
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
        int offset = 0;
        for (auto c : ui.label) {
          if (c == ' ') {
            offset += 11; continue;
          }

          std::string sc = "SMALL_";
          sc += c;
          uiAnimator.SetAnimation(sc);
          uiAnimator.SetFrame(1, uiSprite);
          uiSprite.setScale(ui.scale);
          uiSprite.setPosition(2.f*(ui.position.x + offset), ui.position.y);

          if (ui.type == uiData::ActionItemType::BATTLE) {
            uiSprite.setColor(sf::Color(255, 165, 0, ui.alpha));
          }
          else {
            uiSprite.setColor(sf::Color(255, 10, 255, ui.alpha));
          }

          ENGINE.Draw(uiSprite);
          offset += (int)uiSprite.getLocalBounds().width + 2;
        }
      }
    }
  }

  for (auto ui : boundKeys) {
    int offset = 0;

    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11; continue;
      }

      std::string sc = "SMALL_";
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition(2.f*(ui.position.x + offset), ui.position.y);

      offset += (int)uiSprite.getLocalBounds().width + 2;

      ENGINE.Draw(uiSprite);

    }

    auto totalOffset = offset;
    offset = 0;

    // 2nd pass for right alignment adjustment
    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11;
       continue;
      }

      std::string sc = "SMALL_";
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition((ui.position.x + 2.0f*(offset - totalOffset)), ui.position.y);

      if (ui.type == uiData::ActionItemType::BATTLE) {
        uiSprite.setColor(sf::Color(255, 165, 0, ui.alpha));
      }
      else {
        uiSprite.setColor(sf::Color(255, 10, 255, ui.alpha));
      }

      ENGINE.Draw(uiSprite);
      offset += (int)uiSprite.getLocalBounds().width + 2;
    }
  }

  ENGINE.Draw(audioBGM);
  ENGINE.Draw(audioSFX);
}

void ConfigScene::onStart()
{
  uiAnimator.SetAnimation("GBA");
  uiAnimator.SetFrame(2, gba);
  AUDIO.Stream("resources/loops/config.ogg", false);
}

void ConfigScene::onLeave()
{
  AUDIO.StopStream();
}

void ConfigScene::onExit()
{
}

void ConfigScene::onEnter()
{
  AUDIO.StopStream();
}

void ConfigScene::onResume()
{
}

void ConfigScene::onEnd()
{
}
