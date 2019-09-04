#include "bnConfigScene.h"
#include <Swoosh\ActivityController.h>
#include "Segues\ZoomFadeIn.h"

ConfigScene::ConfigScene(swoosh::ActivityController &controller) : swoosh::Activity(&controller)
{
  // Draws the scrolling background
  bg = new LanBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  // ui sprite maps
  // ascii 58 - 96
  std::list<std::string> frames;

  frames.push_back("MOVE UP");
  frames.push_back("MOVE LEFT");
  frames.push_back("MOVE RIGHT");
  frames.push_back("MOVE DOWN");
  frames.push_back("SHOOT");
  frames.push_back("USE CHIP");
  frames.push_back("SP. ATTACK");
  frames.push_back("CUST MENU");
  frames.push_back("PAUSE");

  //for (int i = 58; i <= 96; i++) {
  for (int i = 65; i <= 90; i++) {
    std::string str;
    str += char(i);
    frames.push_back(str);
  }

  uiAnimator = Animation("resources/backgrounds/config/ui_compressed.animation");
  uiAnimator.Load();

  cursorAnimator = uiAnimator;

  auto sprite = sf::Sprite(LOAD_TEXTURE(CONFIG_UI));
  sprite.setScale(2.f, 2.f);

  uiSprite = sprite;

  // audio button
  audio = sprite;
  uiAnimator.SetAnimation("AUDIO_BG");
  uiAnimator.Update(0, audio);
  audio.setPosition(2*3, 2*140);

  // GBA
  gba = sprite;
  uiAnimator.SetAnimation("GBA");
  uiAnimator.Update(0, gba);
  gba.setPosition(2*63, 2*47);

  // red cursor
  cursor = sprite;
  cursorAnimator.SetAnimation("CURSOR");
  cursorAnimator.Update(0, cursor);
  cursor.setPosition(2*150, 2*70);
  cursorAnimator << Animate::Mode::Loop;

  // A/B hint
  hint = sprite;
  uiAnimator.SetAnimation("HINT");
  uiAnimator.Update(0, hint);
  hint.setPosition(2*30, 2*140);

  points = { {76, 54}, {164, 54},  {84, 92}, {85, 98}, {69,77}, {76,69}, {
    83, 77}, {76, 83}, {169, 74}, {158, 78}, {160, 96} };

  for (auto f : frames) {
    uiList.push_back({ f, sf::Vector2f(), sf::Vector2f() });
    boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f() });
  }

  leave = false;
  awaitingKey = false;
  gotoNextScene = true; // true when entering or leaving, prevents user from interacting with scene

  menuSelectionIndex = lastMenuSelectionIndex = 0;
  maxMenuSelectionIndex = (int)frames.size();

  audioMode = 3; // full
}

void ConfigScene::onUpdate(double elapsed)
{
  bg->Update((float)elapsed);
  cursorAnimator.Update((float)elapsed, cursor);

  auto p = points[std::min(menuSelectionIndex, (int)points.size() - 1)];

  cursor.setPosition(p.x*2.f, p.y*2.f);

  if (INPUT.Has(InputEvent::PRESSED_B) && !leave) {
    if (!awaitingKey) {
      using namespace swoosh::intent;
      using effect = segue<ZoomFadeIn>;
      getController().queuePop<effect>();
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      leave = true;
    }
  }

  if (!leave) {
    if (awaitingKey) {
      auto key = INPUT.GetAnyKey();

      if (key != sf::Keyboard::Unknown) {
        std::string boundKey = "";

        if (INPUT.ConvertKeyToString(key, boundKey)) {
          std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);
          boundKeys[menuSelectionIndex].label = boundKey;

          awaitingKey = false;
        }
      }
    }
    else if (INPUT.Has(InputEvent::PRESSED_UP)) {
      menuSelectionIndex--;
    } else if (INPUT.Has(InputEvent::PRESSED_DOWN)) {
      menuSelectionIndex++;
    }
    else if (INPUT.Has(InputEvent::PRESSED_A)) {
      if (menuSelectionIndex == maxMenuSelectionIndex) {
        audioMode = (audioMode+1) % 4;

        // only mute fx if all audio is mute
        if (audioMode == 0) {
          AUDIO.SetChannelVolume(0);
        }
        else {
          AUDIO.SetChannelVolume(100);
        }

        //AUDIO.SetChannelVolume(((audioMode)/3.0f)*100.0f);
        AUDIO.SetStreamVolume (((audioMode)/3.0f)*100.0f);
        uiAnimator.SetAnimation("AUDIO");
        uiAnimator.SetFrame(audioMode + 1, audio);
      }
      else if(!awaitingKey) {
        awaitingKey = true;
        AUDIO.Play(AudioType::CHIP_CONFIRM);
      }
    }
  }

  menuSelectionIndex = std::max(0, menuSelectionIndex);
  menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);

  if (lastMenuSelectionIndex != menuSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_SELECT);

    lastMenuSelectionIndex = menuSelectionIndex;
  }

  lastMenuSelectionIndex = menuSelectionIndex;

  for (int i = 0; i < maxMenuSelectionIndex; i++) {
    auto w = 0.3f;
    auto diff = i - menuSelectionIndex;
    float scale = 1.0f - (w*abs(diff));
    scale = std::max(scale, 0.3f);
    scale = std::min(scale, 1.0f);

    //scale = 1.0f;
    auto starty = 40;
    auto delta = 48.0f *float(elapsed);

    auto s = sf::Vector2f(2.f*scale, 2.f*scale);
    auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, uiList[i].scale.x), swoosh::ease::interpolate(delta, s.y, uiList[i].scale.y));
    uiList[i].scale = slerp;
    boundKeys[i].scale = slerp;

    // recalculate, unbounded this time
    scale = 1.0f - (w*abs(diff));
    scale = std::max(scale, 0.3f);
    scale = std::min(scale, 1.5f);

    s = sf::Vector2f(2.f*scale, 2.f*scale);

    auto limit = std::min(menuSelectionIndex, maxMenuSelectionIndex - 10); // stop the screen from rolling up when reaching this point of the list
    auto pos = sf::Vector2f(2.f * (5 + (3 * s.x)), 2.f*(starty + (i * 10) - (limit * 10)));
    auto lerp = sf::Vector2f(swoosh::ease::interpolate(delta, pos.x, uiList[i].position.x), swoosh::ease::interpolate(delta, pos.y, uiList[i].position.y));
    uiList[i].position = lerp;
    boundKeys[i].position.x = 240 * 2.f;
    boundKeys[i].position.y = lerp.y;

    if (i != menuSelectionIndex) {
      uiList[i].alpha = boundKeys[i].alpha = 100;

      if (awaitingKey) {
        uiList[i].alpha = boundKeys[i].alpha = 50;
      }
    }
    else {
      uiList[i].alpha = boundKeys[i].alpha = 255;
    }
  }

  // max is reserved for the audio widget
  if (menuSelectionIndex != maxMenuSelectionIndex) {
    audio.setColor(sf::Color(255, 255, 255, 100));
  }
  else {
    audio.setColor(sf::Color(255, 255, 255, 255));
  }
}

void ConfigScene::onDraw(sf::RenderTexture & surface)
{
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(bg);

  ENGINE.Draw(gba);
  ENGINE.Draw(hint);
  ENGINE.Draw(cursor);
  ENGINE.Draw(audio);

  for (auto ui : uiList) {
    int offset = 0;
    for (auto c : ui.label) {
      if (c == ' ') {
        offset+=11; continue;
      }

      std::string sc;
      sc += c;
      uiAnimator.SetAnimation(sc);
      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition(ui.scale.x*(ui.position.x + offset), ui.position.y);
      uiSprite.setColor(sf::Color(255, 255, 255, ui.alpha));
      ENGINE.Draw(uiSprite);
      offset += uiSprite.getLocalBounds().width;
    }
  }

  for (auto ui : boundKeys) {
    int offset = 0;

    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11; continue;
      }

      std::string sc;
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition((ui.position.x + offset), ui.position.y);
      //ENGINE.Draw(uiSprite);
      offset += uiSprite.getLocalBounds().width;
    } 
    auto totalOffset = offset;
    offset = 0;

    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11;
       continue;
      }

      std::string sc;
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition((ui.position.x - (totalOffset)) + offset, ui.position.y);
      uiSprite.setColor(sf::Color(255, 255, 255, ui.alpha));
      ENGINE.Draw(uiSprite);
      offset += uiSprite.getLocalBounds().width;
    }
  }

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
  uiList.clear();
}
