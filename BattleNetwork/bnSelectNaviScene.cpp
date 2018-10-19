#include <time.h>
#include "bnSelectNaviScene.h"
#include "bnGridBackground.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnAnimation.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnTextBox.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#define UI_LEFT_POS_MAX 10.f
#define UI_RIGHT_POS_MAX 300.f
#define UI_TOP_POS_MAX 0.f
#define UI_SPACING 55.f

#define UI_LEFT_POS_START -300.f
#define UI_RIGHT_POS_START 640.f
#define UI_TOP_POS_START 250.f

#define MAX_PIXEL_FACTOR 125.f

SelectedNavi SelectNaviScene::Run(SelectedNavi currentNavi) {
  Camera camera(ENGINE.GetDefaultView());

  // Menu name font
  sf::Font* font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  sf::Text* menuLabel = new sf::Text("BATTLE SELECT", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  // Selection input delays
  double maxSelectInputCooldown = 0.5; // half of a second
  double selectInputCooldown = maxSelectInputCooldown;

  // NAVI UI font
  sf::Font *naviFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  sf::Text *naviLabel = new sf::Text("No Data", *naviFont);
  naviLabel->setPosition(30.f, 18.f);
  naviLabel->setOutlineColor(sf::Color(48, 56, 80));
  naviLabel->setOutlineThickness(2.f);
  naviLabel->setScale(0.8f, 0.8f);

  sf::Text *attackLabel = new sf::Text("1", *naviFont);
  attackLabel->setPosition(335.f, 15.f);
  attackLabel->setOutlineColor(sf::Color(48, 56, 80));
  attackLabel->setOutlineThickness(2.f);
  attackLabel->setScale(0.8f, 0.8f);

  sf::Text *speedLabel = new sf::Text("1", *naviFont);
  speedLabel->setPosition(335.f, 70.f);
  speedLabel->setOutlineColor(sf::Color(48, 56, 80));
  speedLabel->setOutlineThickness(2.f);
  speedLabel->setScale(0.8f, 0.8f);

  sf::Text *hpLabel = new sf::Text("20", *naviFont);
  hpLabel->setOutlineColor(sf::Color(48, 56, 80));
  hpLabel->setPosition(sf::Vector2f(335.f, 125.0f));
  hpLabel->setOutlineThickness(2.f);
  hpLabel->setScale(0.8f, 0.8f);

  float maxNumberCooldown = 0.5;
  float numberCooldown = maxNumberCooldown; // half a second
  SelectedNavi naviSelectionIndex = currentNavi;
  SelectedNavi prevChosen = currentNavi;
  // select menu graphic
  Background* bg = new GridBackground();

  // UI Sprites
  float UI_RIGHT_POS = UI_RIGHT_POS_START;
  float UI_LEFT_POS = UI_LEFT_POS_START;
  float UI_TOP_POS = UI_TOP_POS_START;

  sf::Sprite charName(LOAD_TEXTURE(CHAR_NAME));
  charName.setScale(2.f, 2.f);
  charName.setPosition(UI_LEFT_POS, 10);

  sf::Sprite charElement(LOAD_TEXTURE(CHAR_ELEMENT));
  charElement.setScale(2.f, 2.f);
  charElement.setPosition(UI_LEFT_POS, 80);

  sf::Sprite charStat(LOAD_TEXTURE(CHAR_STAT));
  charStat.setScale(2.f, 2.f);
  charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);

  sf::Sprite charInfo(LOAD_TEXTURE(CHAR_INFO_BOX));
  charInfo.setScale(2.f, 2.f);
  charInfo.setPosition(UI_RIGHT_POS, 170);

  sf::Sprite element(*TEXTURES.GetTexture(TextureType::ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(UI_LEFT_POS_MAX + 15.f, 90);

  // Current navi graphic
  bool loadNavi = false;
  sf::Sprite navi(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
  navi.setScale(2.f, 2.f);
  navi.setOrigin(navi.getLocalBounds().width / 2.f, navi.getLocalBounds().height / 2.f);
  navi.setPosition(100.f, 150.f);

  // Animator for navi
  Animation naviAnimator;

  navi.setTexture(NAVIS.At(naviSelectionIndex).GetBattleTexture());
  naviLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetName().c_str()));
  speedLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetSpeedString().c_str()));
  attackLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetAttackString().c_str()));
  hpLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetHPString().c_str()));

  naviAnimator.SetFrame(1, &navi);

  // Distortion effect
  double factor = MAX_PIXEL_FACTOR;

  // Transition
  sf::Shader& transition = LOAD_SHADER(TRANSITION);
  transition.setUniform("texture", sf::Shader::CurrentTexture);
  transition.setUniform("map", LOAD_TEXTURE(NOISE_TEXTURE));
  transition.setUniform("progress", 0.f);
  float transitionProgress = 0.9f;
  ENGINE.RevokeShader();

  bool gotoNextScene = false;

  SmartShader pixelated;
  pixelated = LOAD_SHADER(TEXEL_PIXEL_BLUR);

  // Load glowing pad animation (never changes/always plays)
  Animation glowpadAnimator("resources/backgrounds/select/glow_pad.animation");
  glowpadAnimator.Load();
  glowpadAnimator.SetAnimation("GLOW");
  glowpadAnimator << Animate::Mode(Animate::Mode::Loop);

  sf::Sprite glowpad(LOAD_TEXTURE(GLOWING_PAD_ATLAS));
  glowpad.setScale(2.f, 2.f);
  glowpad.setPosition(37, 135);

  sf::Sprite glowbase(LOAD_TEXTURE(GLOWING_PAD_BASE));
  glowbase.setScale(2.f, 2.f);
  glowbase.setPosition(40, 200);

  sf::Sprite glowbottom(LOAD_TEXTURE(GLOWING_PAD_BOTTOM));
  glowbottom.setScale(2.f, 2.f);
  glowbottom.setPosition(40, 200);

  // Text box 
  TextBox textbox(135, 15);
  textbox.SetSpeed(15);
  textbox.setPosition(UI_RIGHT_POS_MAX + 10, 205);
  textbox.Stop();

  // Timers and clocks
  Clock clock;
  float elapsed = 0.0f;
  float totalTime = 0.f;

  while (ENGINE.Running()) {

    float elapsedSeconds = clock.restart().asSeconds();
    totalTime += elapsedSeconds;

    float FPS = 0.f;

    if (elapsedSeconds > 0.f) {
      FPS = 1.0f / elapsedSeconds;
      std::string fpsStr = std::to_string(FPS);
      fpsStr.resize(4);
      ENGINE.GetWindow()->setTitle(sf::String(std::string("FPS: ") + fpsStr));
    }

    INPUT.update();

    ENGINE.Clear();
    //ENGINE.SetView(camera.GetView());

    camera.Update(elapsed);
    textbox.Update(elapsed);

    ENGINE.Draw(bg);
    ENGINE.Draw(glowbottom);
    ENGINE.Draw(glowbase);

    charName.setPosition(UI_LEFT_POS, charName.getPosition().y);
    ENGINE.Draw(charName);

    charElement.setPosition(UI_LEFT_POS, charElement.getPosition().y);
    ENGINE.Draw(charElement);

    // Draw stat box three times for three diff. properties
    float charStat1Max = 10;

    if (UI_TOP_POS < charStat1Max)
      charStat.setPosition(UI_RIGHT_POS, charStat1Max);
    else
      charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
    ENGINE.Draw(charStat);

    // 2nd stat box
    float charStat2Max = 10 + UI_SPACING;

    if (UI_TOP_POS < charStat2Max)
      charStat.setPosition(UI_RIGHT_POS, charStat2Max);
    else
      charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
    ENGINE.Draw(charStat);

    // 3rd stat box
    float charStat3Max = 10 + (UI_SPACING * 2);

    if (UI_TOP_POS < charStat3Max)
      charStat.setPosition(UI_RIGHT_POS, charStat3Max);
    else
      charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
    ENGINE.Draw(charStat);

    // SP. Info box
    charInfo.setPosition(UI_RIGHT_POS, charInfo.getPosition().y);
    ENGINE.Draw(charInfo);

    // Draw glow pad behind everything 
    glowpadAnimator.Update(elapsed, &glowpad);
    ENGINE.Draw(glowpad);

    ENGINE.DrawUnderlay();
    ENGINE.DrawLayers();
    ENGINE.DrawOverlay();

    naviAnimator.Update(elapsed, &navi);
    bg->Update(elapsed);

    SelectedNavi prevSelect = naviSelectionIndex;

    // Scene keyboard controls
    if (!gotoNextScene && transitionProgress == 0.f) {
      if (INPUT.has(PRESSED_LEFT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to previous mob 
          selectInputCooldown = maxSelectInputCooldown;
          naviSelectionIndex = static_cast<SelectedNavi>((int)naviSelectionIndex - 1);

          // Number scramble effect
          numberCooldown = maxNumberCooldown;
        }
      }
      else if (INPUT.has(PRESSED_RIGHT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to next mob 
          selectInputCooldown = maxSelectInputCooldown;
          naviSelectionIndex = static_cast<SelectedNavi>((int)naviSelectionIndex + 1);

          // Number scramble effect
          numberCooldown = maxNumberCooldown;
        }
      }
      else {
        selectInputCooldown = 0;
      }

      if (INPUT.has(PRESSED_B)) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
        textbox.Mute();
      }

      if (INPUT.has(PRESSED_UP)) {
        textbox.ShowPreviousLine();
      }

      if (INPUT.has(PRESSED_DOWN)) {
        textbox.ShowNextLine();
      }
    }

    if (elapsed > 0) {
      if (gotoNextScene) {
        if (UI_TOP_POS > 250 && UI_LEFT_POS < -300) {
          transitionProgress += 1 * elapsed;
        }
      }
      else {
        transitionProgress -= 1 * elapsed;
      }
    }

    transitionProgress = std::max(0.f, transitionProgress);
    transitionProgress = std::min(1.f, transitionProgress);

    if (transitionProgress >= 1.f) {
      break;
    }

    naviSelectionIndex = (SelectedNavi)std::max(0, (int)naviSelectionIndex);
    naviSelectionIndex = (SelectedNavi)std::min((int)NAVIS.Size() - 1, (int)naviSelectionIndex);

    if (naviSelectionIndex != prevSelect || !loadNavi) {
      factor = 125;

      naviAnimator = Animation(NAVIS.At(naviSelectionIndex).GetBattleAnimationPath());
      naviAnimator.Load();
      naviAnimator.SetAnimation("PLAYER_IDLE");
      naviAnimator << Animate::Mode(Animate::Mode::Loop);

      int offset = (int)(NAVIS.At(naviSelectionIndex).GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));

      navi.setTexture(NAVIS.At(naviSelectionIndex).GetBattleTexture(), true);
      textbox.SetMessage(NAVIS.At(naviSelectionIndex).GetSpecialDescriptionString());
      loadNavi = true;
    }

    // This goes here because the jumbling effect may finish and we need to see proper values
    naviLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetName()));
    speedLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetSpeedString()));
    attackLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetAttackString()));
    hpLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetHPString()));

    if (numberCooldown > 0) {
      numberCooldown -= elapsed;
      std::string newstr;

      for (int i = 0; i < naviLabel->getString().getSize(); i++) {
        double progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;
        double index = progress * naviLabel->getString().getSize();

        if (i < (int)index) {
          newstr += naviLabel->getString()[i];
        }
        else {
          if (naviLabel->getString()[i] != ' ') {
            newstr += (char)(((rand() % (90 - 65)) + 65) + 1);
          }
          else {
            newstr += ' ';
          }
        }
      }

      int randAttack = rand() % 10;
      int randSpeed = rand() % 10;

      //attackLabel->setString(std::to_string(randAttack));
      //speedLabel->setString(std::to_string(randSpeed));
      naviLabel->setString(sf::String(newstr));
    }

    float progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;

    if (progress > 1.f) progress = 1.f;

    // Update UI slide in
    if (!gotoNextScene) {
      factor -= elapsed * 180.f;

      if (factor <= 0.f) {
        factor = 0.f;
      }

      if (UI_RIGHT_POS > UI_RIGHT_POS_MAX) {
        UI_RIGHT_POS -= elapsed * 500;
      }
      else {
        UI_RIGHT_POS = UI_RIGHT_POS_MAX;
        UI_TOP_POS -= elapsed * 500;

        if (UI_TOP_POS < UI_TOP_POS_MAX) {
          UI_TOP_POS = UI_TOP_POS_MAX;

          // Draw labels
          ENGINE.Draw(naviLabel);
          ENGINE.Draw(hpLabel);
          ENGINE.Draw(speedLabel);
          ENGINE.Draw(attackLabel);
          ENGINE.Draw(textbox);
          ENGINE.Draw(element);

          textbox.Play();
        }
      }

      if (UI_LEFT_POS < UI_LEFT_POS_MAX) {
        static bool once = true;

        if (once) {
          AUDIO.Play(AudioType::TOSS_ITEM); once = false;
        }

        UI_LEFT_POS += elapsed * 500;
      }
      else {
        UI_LEFT_POS = UI_LEFT_POS_MAX;
      }
    }
    else {
      factor += elapsed * 180.f;

      if (factor >= MAX_PIXEL_FACTOR) {
        factor = MAX_PIXEL_FACTOR;
      }

      if (UI_TOP_POS < UI_TOP_POS_START) {
        UI_TOP_POS += elapsed * 500;
      }
      else {
        UI_RIGHT_POS += elapsed * 500;

        if (UI_RIGHT_POS > UI_RIGHT_POS_START / 2) // Be quicker at leave than startup
          UI_LEFT_POS -= elapsed * 500;
      }
    }

    if (prevChosen != naviSelectionIndex) {
      navi.setColor(sf::Color(200, 200, 200, 128));
    }
    else {
      navi.setColor(sf::Color(255, 255, 255, 255));
    }

    if (factor != 0.f) {
      float range = (125.f - (float)factor) / 125.f;
      navi.setColor(sf::Color(255, 255, 255, (sf::Uint8)(navi.getColor().a * range)));
    }

    sf::IntRect t = navi.getTextureRect();
    sf::Vector2u size = navi.getTexture()->getSize();
    pixelated.SetUniform("x", (float)t.left / (float)size.x);
    pixelated.SetUniform("y", (float)t.top / (float)size.y);
    pixelated.SetUniform("w", (float)t.width / (float)size.x);
    pixelated.SetUniform("h", (float)t.height / (float)size.y);
    pixelated.SetUniform("pixel_threshold", (float)(factor / 400.f));

    // Refresh mob graphic origin every frame as it may change
    float xpos = ((glowbase.getTextureRect().width / 2.0f)*glowbase.getScale().x) + glowbase.getPosition().x;
    navi.setPosition(xpos, glowbase.getPosition().y+10);

    LayeredDrawable* bake = new LayeredDrawable(sf::Sprite(navi));
    bake->SetShader(pixelated);

    ENGINE.Draw(bake);
    delete bake;

    // Make a selection
    if (INPUT.has(PRESSED_A) && prevChosen != naviSelectionIndex) {
      AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::LOW);
      prevChosen = prevSelect;

      // TODO: Highlight the chosen navi symbol
    } 

    sf::Texture postprocessing; // = ENGINE.GetPostProcessingBuffer().getTexture(); // Make a copy
    sf::Sprite transitionPost;
    transitionPost.setTexture(postprocessing);

    transition.setUniform("progress", transitionProgress);

    bake = new LayeredDrawable(transitionPost);
    bake->SetShader(&transition);

    ENGINE.Draw(bake);
    delete bake;

    // Write contents to screen (always last step)
    // ENGINE.Display();

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }
  delete font;
  delete naviFont;
  delete naviLabel;
  delete attackLabel;
  delete speedLabel;
  delete menuLabel;
  delete hpLabel;
  delete bg;

  ENGINE.RevokeShader();

  return prevChosen;
}