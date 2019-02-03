#include <Swoosh\ActivityController.h>
#include "bnSelectMobScene.h"

SelectMobScene::SelectMobScene(swoosh::ActivityController& controller, SelectedNavi navi, ChipFolder& selectedFolder) :
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

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // MOB UI font
  mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  mobLabel = new sf::Text("Mettaur", *mobFont);
  mobLabel->setPosition(275.f, 15.f);

  attackLabel = new sf::Text("1", *mobFont);
  attackLabel->setPosition(325.f, 30.f);

  speedLabel = new sf::Text("1", *mobFont);
  speedLabel->setPosition(325.f, 45.f);

  hpFont = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  hpLabel = new sf::Text("20", *hpFont);
  hpLabel->setOutlineColor(sf::Color(48, 56, 80));
  hpLabel->setOutlineThickness(2.f);
  hpLabel->setScale(0.8f, 0.8f);
  hpLabel->setOrigin(hpLabel->getLocalBounds().width, 0);
  hpLabel->setPosition(sf::Vector2f(180.f, 33.0f));

  maxNumberCooldown = 0.5;
  numberCooldown = maxNumberCooldown; // half a second

  // select menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(BATTLE_SELECT_BG));
  bg.setScale(2.f, 2.f);

  // Current mob graphic
  mobSpr = sf::Sprite(LOAD_TEXTURE(MOB_METTAUR));
  mobSpr.setScale(2.f, 2.f);
  mobSpr.setOrigin(mobSpr.getLocalBounds().width / 2.f, mobSpr.getLocalBounds().height / 2.f);
  mobSpr.setPosition(110.f, 130.f);

  mobAnimator = Animation("resources/mobs/mettaur/mettaur.animation");
  mobAnimator.Reload();
  mobAnimator.SetAnimation("IDLE");
  mobAnimator.SetFrame(1, &mobSpr);

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
  if(factory) delete factory;
  if(field) delete field;

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
  navigatorAnimator.Update((float)elapsed, &navigator);

  camera.Update((float)elapsed);
  textbox.Update((float)elapsed);

  int prevSelect = mobSelectionIndex;

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUT.has(PRESSED_LEFT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous mob 
        selectInputCooldown = maxSelectInputCooldown;
        mobSelectionIndex--;

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else if (INPUT.has(PRESSED_RIGHT)) {
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

    if (INPUT.has(PRESSED_B)) {
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

      using segue = swoosh::intent::segue<BlackWashFade>;

      getController().queuePop<segue>();
    }
  }

  mobSelectionIndex = std::max(0, mobSelectionIndex);
  mobSelectionIndex = std::min(3, mobSelectionIndex);

  if (mobSelectionIndex == 0) {
    mobLabel->setString("Mettaur");
    speedLabel->setString("2");
    attackLabel->setString("1");
    hpLabel->setString("20");
  }
  else if (mobSelectionIndex == 1) {
    mobLabel->setString("ProgsMan");
    speedLabel->setString("4");
    attackLabel->setString("3");
    hpLabel->setString("300");
  }
  else if (mobSelectionIndex == 2) {
    mobLabel->setString("Canodumb");
    speedLabel->setString("1");
    attackLabel->setString("4");
    hpLabel->setString("60");
  }
  else {
    mobLabel->setString("Random Mob");
    speedLabel->setString("*");
    attackLabel->setString("*");
    hpLabel->setString("");
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

    attackLabel->setString(std::to_string(randAttack));
    speedLabel->setString(std::to_string(randSpeed));
    mobLabel->setString(sf::String(newstr));
  }

  if (mobSelectionIndex != prevSelect || doOnce) {
    doOnce = false;
    factor = 125;

    if (mobSelectionIndex == 0) {
      mobSpr.setTexture(*TEXTURES.GetTexture(TextureType::MOB_METTAUR), true);
      mobSpr.setPosition(110.f, 130.f);

      mobAnimator = Animation("resources/mobs/mettaur/mettaur.animation");
      mobAnimator.Reload();
      mobAnimator.SetAnimation("IDLE");
      mobAnimator.SetFrame(1, &mobSpr);

      textbox.SetMessage("Tutorial ranked Mettaurs. You got this!");
    }
    else if (mobSelectionIndex == 1) {
      mobSpr.setTexture(*TEXTURES.GetTexture(TextureType::MOB_PROGSMAN_ATLAS), true);
      mobSpr.setPosition(100.f, 110.f);

      mobAnimator = Animation("resources/mobs/progsman/progsman.animation");
      mobAnimator.Reload();
      mobAnimator.SetAnimation(MOB_IDLE);
      mobAnimator.SetFrame(1, &mobSpr);

      textbox.SetMessage("A rogue Mr. Prog that became too strong to control.");
    }
    else if (mobSelectionIndex == 2) {
      mobSpr.setTexture(*TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
      mobSpr.setPosition(100.f, 130.f);

      mobAnimator = Animation("resources/mobs/canodumb/canodumb.animation");
      mobAnimator.Reload();
      mobAnimator.SetAnimation(MOB_CANODUMB_IDLE_1);
      mobAnimator.SetFrame(1, &mobSpr);

      textbox.SetMessage("A family of cannon based virii. Watch out!");
    }
    else {
      mobSpr.setTexture(*TEXTURES.GetTexture(TextureType::MOB_ANYTHING_GOES), true);
      mobSpr.setPosition(110.f, 130.f);
      textbox.SetMessage("A randomly generated mob and field. Anything goes.");
    }
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
  if (INPUT.has(PRESSED_A) && !gotoNextScene) {

    gotoNextScene = true;

    AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::LOWEST);

    // Stop music and go to battle screen 
    AUDIO.StopStream();

    field = new Field(6, 3);

    if (mobSelectionIndex == 0) {
      // see how the random mob works around holes
      field->GetAt((rand()) % 3 + 4, (rand() % 3) + 1)->SetState(TileState::EMPTY);
      factory = new TwoMettaurMob(field);
    }
    else if (mobSelectionIndex == 1) {
      factory = new ProgsManBossFight(field);
    }
    else if (mobSelectionIndex == 2) {
      factory = new CanodumbMob(field);
    }
    else {
      factory = new RandomMettaurMob(field);
    }

    mob = factory->Build();


    Player* player = NAVIS.At(selectedNavi).GetNavi();
    
    // Folder is owned and deleted by the chip cust
    ChipFolder* folder = selectedFolder.Clone();
    folder->Shuffle();

    using segue = swoosh::intent::segue<CrossZoom>::to<BattleScene>;
    getController().push<segue>(player, mob, folder);
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
  mobLabel->setPosition(277.f, 17.f);
  mobLabel->setFillColor(sf::Color(138, 138, 138));
  ENGINE.Draw(mobLabel);
  mobLabel->setPosition(275.f, 15.f);
  mobLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(mobLabel);

  // Draw attack rating with shadow
  attackLabel->setPosition(432.f, 48.f);
  attackLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(attackLabel);
  attackLabel->setPosition(430.f, 46.f);
  attackLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(attackLabel);

  // Draw speed rating with shadow
  speedLabel->setPosition(432.f, 80.f);
  speedLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(speedLabel);
  speedLabel->setPosition(430.f, 78.f);
  speedLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(speedLabel);

  // Draw hp
  ENGINE.Draw(hpLabel);

  ENGINE.DrawUnderlay();
  ENGINE.DrawLayers();
  ENGINE.DrawOverlay();

  sf::IntRect t = mobSpr.getTextureRect();
  sf::Vector2u size = mobSpr.getTexture()->getSize();
  shader.SetUniform("x", (float)t.left / (float)size.x);
  shader.SetUniform("y", (float)t.top / (float)size.y);
  shader.SetUniform("w", (float)t.width / (float)size.x);
  shader.SetUniform("h", (float)t.height / (float)size.y);
  shader.SetUniform("pixel_threshold", (float)(factor / 400.f));


  // Refresh mob graphic origin every frame as it may change
  mobSpr.setOrigin(mobSpr.getTextureRect().width / 2.f, mobSpr.getTextureRect().height / 2.f);
  hpLabel->setOrigin(hpLabel->getLocalBounds().width, 0);

  LayeredDrawable* bake = new LayeredDrawable(sf::Sprite(mobSpr));
  bake->SetShader(shader);

  if (showMob) {
    ENGINE.Draw(bake);
    delete bake;
  }

  ENGINE.Draw(textbox);
  ENGINE.Draw(navigator);
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