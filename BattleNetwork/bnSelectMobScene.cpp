#include <Swoosh\ActivityController.h>
#include "bnSelectMobScene.h"

SelectMobScene::SelectMobScene(swoosh::ActivityController& controller, SelectedNavi navi, ChipFolder& selectedFolder) :
  elapsed(0),
  camera(ENGINE.GetDefaultView()),
  textbox(320, 100, 24, "resources/fonts/NETNAVI_4-6_V3.ttf"),
  selectedFolder(selectedFolder),
  swoosh::Activity(controller)
{
  selectedNavi = navi;

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("BATTLE SELECT", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  navigator = sf::Sprite(LOAD_TEXTURE(MUG_NAVIGATOR));
  navigator.setScale(2.0f, 2.0f);
  navigator.setPosition(10.0f, 208.0f);

  navigatorAnimator = Animation("resources/ui/navigator.animation");
  navigatorAnimator.Reload();
  navigatorAnimator.SetAnimation("TALK");
  navigatorAnimator << Animate::Mode::Loop;

  mobSpr = sf::Sprite();

  cursor = sf::Sprite(LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // MOB UI font
  mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  mobLabel = new sf::Text("", *mobFont);
  mobLabel->setPosition(sf::Vector2f(100.f, 45.0f));

  attackLabel = new sf::Text("", *mobFont);
  attackLabel->setPosition(325.f, 30.f);

  speedLabel = new sf::Text("", *mobFont);
  speedLabel->setPosition(325.f, 45.f);

  hpLabel = new sf::Text("", *mobFont);
  hpLabel->setPosition(325.f, 60.f);

  maxNumberCooldown = 0.5;
  numberCooldown = maxNumberCooldown; // half a second

  // select menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(BATTLE_SELECT_BG));
  bg.setScale(2.f, 2.f);

  gotoNextScene = true; 
  doOnce = false; // wait until the scene starts or resumes
  showMob = false;

  shader = LOAD_SHADER(TEXEL_PIXEL_BLUR);
  factor = 125;

  // Current selection index
  mobSelectionIndex = 0;

  // Text box navigator
  textbox.Stop();
  textbox.setPosition(100, 210);
  textbox.SetTextColor(sf::Color::Black);
  textbox.SetCharactersPerSecond(25);
}

SelectMobScene::~SelectMobScene() {
  /*delete font;
  delete mobFont;
  delete hpFont;
  delete mobLabel;
  delete attackLabel;
  delete speedLabel;
  delete menuLabel;
  delete hpLabel;*/

  /*if (mob) delete mob;
  if (factory) delete factory;
  if (field) delete field;*/
}

void SelectMobScene::onResume() {
  if(mob) delete mob;

  // Fix camera if offset from battle
  ENGINE.SetCamera(camera);

  // Re-play music
  AUDIO.Stream("resources/loops/loop_navi_customizer.ogg", true);

  gotoNextScene = false;
  doOnce = true;
  showMob = true;
  textbox.Play();
}

void SelectMobScene::onUpdate(double elapsed) {
  this->elapsed += elapsed;

  navigatorAnimator.Update((float)elapsed, navigator);

  camera.Update((float)elapsed);
  textbox.Update((float)elapsed);

  int prevSelect = mobSelectionIndex;

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUT.Has(PRESSED_LEFT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous mob 
        selectInputCooldown = maxSelectInputCooldown;
        mobSelectionIndex--;

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else if (INPUT.Has(PRESSED_RIGHT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to next mob 
        selectInputCooldown = maxSelectInputCooldown;
        mobSelectionIndex++;

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (INPUT.Has(PRESSED_B)) {
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

      using segue = swoosh::intent::segue<BlackWashFade>;

      getController().queuePop<segue>();
    }
  }

  mobSelectionIndex = std::max(0, mobSelectionIndex);
  mobSelectionIndex = std::min((int)MOBS.Size()-1, mobSelectionIndex);

  auto& mobinfo = MOBS.At(mobSelectionIndex);

  mobLabel->setString(mobinfo.GetName());
  hpLabel->setString(mobinfo.GetHPString());
  speedLabel->setString(mobinfo.GetSpeedString());
  attackLabel->setString(mobinfo.GetAttackString());

  if (prevSelect != mobSelectionIndex || doOnce) {
    doOnce = false;
    factor = 125;

    // Current mob graphic
    mobSpr = sf::Sprite(*mobinfo.GetPlaceholderTexture());
    mobSpr.setScale(2.f, 2.f);
    mobSpr.setOrigin(mobSpr.getLocalBounds().width / 2.f, mobSpr.getLocalBounds().height / 2.f);
    mobSpr.setPosition(110.f, 130.f);

    textbox.SetMessage(mobinfo.GetDescriptionString());

    prevSelect = mobSelectionIndex;
  }

  if (numberCooldown > 0) {
    numberCooldown -= (float)elapsed;
    std::string newstr;

    for (int i = 0; i < mobLabel->getString().getSize(); i++) {
      double progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;
      double index = progress * mobLabel->getString().getSize();

      if (i < (int)index) {
        newstr += mobLabel->getString()[i];
      }
      else {
        if (mobLabel->getString()[i] != ' ') {
          newstr += (char)(((rand() % (90 - 65)) + 65) + 1);
        }
        else {
          newstr += ' ';
        }
      }
    }

    int randAttack = rand() % 10;
    int randSpeed = rand() % 10;

    int randHP = 0;

    int count = (int)mobinfo.GetHPString().size() - 1;

    while (count >= 0) {
      int index = (int)std::pow(10.0, (double)count);
      index *= (rand() % 9) + 1;

      randHP += index;

      count--;
    }

    attackLabel->setString(std::to_string(randAttack));
    speedLabel->setString(std::to_string(randSpeed));
    hpLabel->setString(std::to_string(randHP));
    mobLabel->setString(sf::String(newstr));
  }

  factor -= (float)elapsed * 180.f;

  if (factor <= 0.f) {
    factor = 0.f;
  }

  float progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;

  if (progress > 1.f) progress = 1.f;

  mobSpr.setColor(sf::Color(255, 255, 255, (sf::Uint32)(255.0*progress)));

  float range = (125.f - factor) / 125.f;
  mobSpr.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255 * range)));

  // Make a selection
  if (INPUT.Has(PRESSED_A) && !gotoNextScene) {
    Mob* mob = nullptr;
    
    if (MOBS.Size() != 0) {
      mob = MOBS.At(mobSelectionIndex).GetMob();
    }

    if (!mob) {
      gotoNextScene = false;

      AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::LOWEST);

    }
    else {
      gotoNextScene = true;

      AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::LOWEST);

      // Stop music and go to battle screen 
      AUDIO.StopStream();

      Player* player = NAVIS.At(selectedNavi).GetNavi();

      selectedFolder.Shuffle();

      using segue = swoosh::intent::segue<CrossZoom>::to<BattleScene>;
      getController().push<segue>(player, mob, &selectedFolder);
    }
  }

  bool isEqual = textbox.GetCurrentCharacter() == '\0';

  if (isEqual && navigatorAnimator.GetAnimationString() != "IDLE") {
    navigatorAnimator.SetAnimation("IDLE");
    navigatorAnimator << Animate::Mode::Loop;
  }
  else if(!isEqual && navigatorAnimator.GetAnimationString() != "TALK") {
    navigatorAnimator.SetAnimation("TALK");
    navigatorAnimator << Animate::Mode::Loop;
  }
}

void SelectMobScene::onDraw(sf::RenderTexture & surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  // Draw mob name with shadow
  mobLabel->setPosition(sf::Vector2f(30.f, 30.0f));
  mobLabel->setFillColor(sf::Color(138, 138, 138));
  ENGINE.Draw(mobLabel);
  mobLabel->setPosition(sf::Vector2f(30.f, 28.0f));
  mobLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(mobLabel);

  // Draw attack rating with shadow
  attackLabel->setPosition(382.f, 80.f);
  attackLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(attackLabel);
  attackLabel->setPosition(380.f, 78.f);
  attackLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(attackLabel);

  // Draw speed rating with shadow
  speedLabel->setPosition(382.f, 112.f);
  speedLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(speedLabel);
  speedLabel->setPosition(380.f, 110.f);
  speedLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(speedLabel);

  // Draw hp
  hpLabel->setPosition(382.f, 144.f);
  hpLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(hpLabel);
  hpLabel->setPosition(380.f, 142.f);
  hpLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(hpLabel);

  ENGINE.DrawUnderlay();
  ENGINE.DrawLayers();
  ENGINE.DrawOverlay();

  if (mobSpr.getTexture()) {
    sf::IntRect t = mobSpr.getTextureRect();
    sf::Vector2u size = mobSpr.getTexture()->getSize();
    shader.SetUniform("x", (float)t.left / (float)size.x);
    shader.SetUniform("y", (float)t.top / (float)size.y);
    shader.SetUniform("w", (float)t.width / (float)size.x);
    shader.SetUniform("h", (float)t.height / (float)size.y);
    shader.SetUniform("pixel_threshold", (float)(factor / 400.f));


    // Refresh mob graphic origin every frame as it may change
    mobSpr.setOrigin(mobSpr.getTextureRect().width / 2.f, mobSpr.getTextureRect().height / 2.f);

    LayeredDrawable* bake = new LayeredDrawable(sf::Sprite(mobSpr));
    bake->SetShader(shader);

    if (showMob) {
      ENGINE.Draw(bake);
      delete bake;
    }
  }

  ENGINE.Draw(textbox);
  ENGINE.Draw(navigator);

  if (mobSelectionIndex > 0) {
    cursor.setColor(sf::Color::White);
  }
  else {
    cursor.setColor(sf::Color(255, 255, 255, 100));
  }

  auto offset = std::sin(this->elapsed*10.0) * 5;
  cursor.setPosition(23.0f + (float)offset, 130.0f);
  cursor.setScale(-2.f, 2.f);
  ENGINE.Draw(cursor);

  if (mobSelectionIndex < (int)(MOBS.Size() - 1)) {
    cursor.setColor(sf::Color::White);
  } else {
    cursor.setColor(sf::Color(255, 255, 255, 100));
  }

  offset = -std::sin(this->elapsed*10.0) * 5;
  cursor.setPosition(200.0f + (float)offset, 130.0f);
  cursor.setScale(2.f, 2.f);
  ENGINE.Draw(cursor);
}

void SelectMobScene::onStart() {
  textbox.Play();
  factor = 125;
  doOnce = true;
  showMob = true;
  gotoNextScene = false;
}

void SelectMobScene::onLeave() {
  textbox.Stop();
}

void SelectMobScene::onExit() {

}

void SelectMobScene::onEnter() {

}

void SelectMobScene::onEnd() {

}