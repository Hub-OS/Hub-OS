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
  auto frames = { "LPAD", "RPAD", "START", "SELECT", "LEFT", "UP", "RIGHT", "DOWN", "A_BUTTON", "B_BUTTON" };

  uiAnimator = Animation("resources/backgrounds/config/ui_compressed.animation");
  uiAnimator.Load();

  cursorAnimator = uiAnimator;

  auto sprite = sf::Sprite(LOAD_TEXTURE(CONFIG_UI));
  sprite.setScale(2.f, 2.f);

  // audio button
  audio = sprite;
  uiAnimator.SetAnimation("AUDIO");
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

  points = { {76, 54}, {164, 54},  {84, 92}, {85, 98}, {69,77}, {76,69}, {83, 77}, {76, 83}, {169, 74}, {158, 78}, {160, 96} };

  for (auto f : frames) {
    auto ui = new sf::Sprite(sprite);
    uiAnimator.SetAnimation(f);
    uiAnimator.Update(0, *ui);
    uiList.push_back(ui);
  }

  leave = false;
  gotoNextScene = true; // true when entering or leaving, prevents user from interacting with scene

  menuSelectionIndex = lastMenuSelectionIndex = 0;
  maxMenuSelectionIndex = (int)frames.size();

  audioMode = 3; // full
}

void ConfigScene::onUpdate(double elapsed)
{
  bg->Update((float)elapsed);
  cursorAnimator.Update((float)elapsed, cursor);

  auto p = points[menuSelectionIndex];
  cursor.setPosition(p.x*2.f, p.y*2.f);

  if (INPUT.Has(InputEvent::PRESSED_B) && !leave) {
    using namespace swoosh::intent;
    using effect = segue<ZoomFadeIn>;
    getController().queuePop<effect>();
    AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
    leave = true;
  }

  if (!leave) {
    if (INPUT.Has(InputEvent::PRESSED_UP)) {
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
    auto w = 0.2f;
    auto diff = i - menuSelectionIndex;
    float scale = 1.0f - (w*abs(diff));
    scale = std::max(scale, 0.3f);
    scale = std::min(scale, 1.0f);

    //scale = 1.0f;
    auto starty = 40;
    auto delta = 48.0f *float(elapsed);

    auto s = sf::Vector2f(2.f*scale, 2.f*scale);
    auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, uiList[i]->getScale().x), swoosh::ease::interpolate(delta, s.y, uiList[i]->getScale().y));
    uiList[i]->setScale(slerp);

    auto pos = sf::Vector2f(2.f * 3, 2.f*(starty + (i * 10) - (menuSelectionIndex * 10)));
    auto lerp = sf::Vector2f(swoosh::ease::interpolate(delta, pos.x, uiList[i]->getPosition().x), swoosh::ease::interpolate(delta, pos.y, uiList[i]->getPosition().y));
    uiList[i]->setPosition(lerp);

    if (i != menuSelectionIndex) {
      uiList[i]->setColor(sf::Color(255, 255, 255, 100));
    }
    else {
      uiList[i]->setColor(sf::Color(255, 255, 255, 255));
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
    ENGINE.Draw(ui);
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
  for (auto l : uiList) {
    delete l;
  }

  uiList.clear();
}
