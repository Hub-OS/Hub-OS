
#include <Swoosh/ActivityController.h>

#include "bnSelectNaviScene.h"
#include "Segues/Checkerboard.h"

SelectNaviScene::SelectNaviScene(swoosh::ActivityController& controller, SelectedNavi& currentNavi) :
  naviSelectionIndex(currentNavi),
  camera(ENGINE.GetView()),
  textbox(135, 15),
  swoosh::Activity(&controller) {

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("BATTLE SELECT", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // NAVI UI font
  naviFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  naviLabel = new sf::Text("No Data", *naviFont);
  naviLabel->setPosition(30.f, 18.f);
  naviLabel->setOutlineColor(sf::Color(48, 56, 80));
  naviLabel->setOutlineThickness(2.f);
  naviLabel->setScale(0.8f, 0.8f);

  attackLabel = new sf::Text("1", *naviFont);
  attackLabel->setPosition(335.f, 15.f);
  attackLabel->setOutlineColor(sf::Color(48, 56, 80));
  attackLabel->setOutlineThickness(2.f);
  attackLabel->setScale(0.8f, 0.8f);

  speedLabel = new sf::Text("1", *naviFont);
  speedLabel->setPosition(335.f, 70.f);
  speedLabel->setOutlineColor(sf::Color(48, 56, 80));
  speedLabel->setOutlineThickness(2.f);
  speedLabel->setScale(0.8f, 0.8f);

  hpLabel = new sf::Text("20", *naviFont);
  hpLabel->setOutlineColor(sf::Color(48, 56, 80));
  hpLabel->setPosition(sf::Vector2f(335.f, 125.0f));
  hpLabel->setOutlineThickness(2.f);
  hpLabel->setScale(0.8f, 0.8f);

  maxNumberCooldown = 0.5;
  numberCooldown = maxNumberCooldown; // half a second

  prevChosen = currentNavi;
  // select menu graphic
  bg = new GridBackground();

  // UI Sprites
  UI_RIGHT_POS = UI_RIGHT_POS_START;
  UI_LEFT_POS = UI_LEFT_POS_START;
  UI_TOP_POS = UI_TOP_POS_START;

  charName.setTexture(LOAD_TEXTURE(CHAR_NAME));
  charName.setScale(2.f, 2.f);
  charName.setPosition(UI_LEFT_POS, 10);

  charElement.setTexture(LOAD_TEXTURE(CHAR_ELEMENT));
  charElement.setScale(2.f, 2.f);
  charElement.setPosition(UI_LEFT_POS, 80);

  charStat.setTexture(LOAD_TEXTURE(CHAR_STAT));
  charStat.setScale(2.f, 2.f);
  charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);

  charInfo.setTexture(LOAD_TEXTURE(CHAR_INFO_BOX));
  charInfo.setScale(2.f, 2.f);
  charInfo.setPosition(UI_RIGHT_POS, 170);

  element.setTexture(TEXTURES.GetTexture(TextureType::ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(UI_LEFT_POS_MAX + 15.f, 90);

  // Current navi graphic
  loadNavi = false;
  navi.setTexture(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
  navi.setScale(2.f, 2.f);
  navi.setOrigin(navi.getLocalBounds().width / 2.f, navi.getLocalBounds().height / 2.f);
  navi.setPosition(100.f, 150.f);

  navi.setTexture(NAVIS.At(naviSelectionIndex).GetBattleTexture());
  naviLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetName().c_str()));
  speedLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetSpeedString().c_str()));
  attackLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetAttackString().c_str()));
  hpLabel->setString(sf::String(NAVIS.At(naviSelectionIndex).GetHPString().c_str()));

  naviAnimator = Animation(NAVIS.At(naviSelectionIndex).GetBattleAnimationPath());
  naviAnimator.Reload();
  naviAnimator.SetAnimation("PLAYER_IDLE");
  naviAnimator << Animator::Mode::Loop;
   
  // Distortion effect
  factor = MAX_PIXEL_FACTOR;

  gotoNextScene = true;

  pixelated = LOAD_SHADER(TEXEL_PIXEL_BLUR);

  // Load glowing pad animation (never changes/always plays)
  glowpadAnimator = Animation("resources/backgrounds/select/glow_pad.animation");
  glowpadAnimator.Reload();
  glowpadAnimator.SetAnimation("GLOW");
  glowpadAnimator << Animator::Mode::Loop;

  glowpad.setTexture(LOAD_TEXTURE(GLOWING_PAD_ATLAS));
  glowpad.setScale(2.f, 2.f);
  glowpad.setPosition(37, 135);

  glowbase.setTexture(LOAD_TEXTURE(GLOWING_PAD_BASE));
  glowbase.setScale(2.f, 2.f);
  glowbase.setPosition(40, 200);

  glowbottom.setTexture(LOAD_TEXTURE(GLOWING_PAD_BOTTOM));
  glowbottom.setScale(2.f, 2.f);
  glowbottom.setPosition(40, 200);

  // Text box 
  textbox.SetCharactersPerSecond(15);
  textbox.setPosition(UI_RIGHT_POS_MAX + 10, 205);
  textbox.Stop();
  textbox.Mute(); // no tick sound

  elapsed = 0;
}

SelectNaviScene::~SelectNaviScene()
{
  delete font;
  delete naviFont;
  delete naviLabel;
  delete attackLabel;
  delete speedLabel;
  delete menuLabel;
  delete hpLabel;
  delete bg;
}

void SelectNaviScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

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

  ENGINE.Draw(glowpad);

  // Update UI slide in
  if (!gotoNextScene) {
    factor -= (float)elapsed * 180.f;

    if (factor <= 0.f) {
      factor = 0.f;
    }

    if (UI_RIGHT_POS > UI_RIGHT_POS_MAX) {
      UI_RIGHT_POS -= (float)elapsed * 1000;
    }
    else {
      UI_RIGHT_POS = UI_RIGHT_POS_MAX;
      UI_TOP_POS -= (float)elapsed * 1000;

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
      UI_LEFT_POS += (float)elapsed * 1000;
    }
    else {
      UI_LEFT_POS = UI_LEFT_POS_MAX;
    }
  }
  else {
    factor += (float)elapsed * 320.f;

    if (factor >= MAX_PIXEL_FACTOR) {
      factor = MAX_PIXEL_FACTOR;
    }

    if (UI_TOP_POS < UI_TOP_POS_START) {
      UI_TOP_POS += (float)elapsed * 1000;
    }
    else {
      UI_RIGHT_POS += (float)elapsed * 1000;

      if (UI_RIGHT_POS > UI_RIGHT_POS_START / 2) // Be quicker at leave than startup
        UI_LEFT_POS -= (float)elapsed * 1000;
    }
  }

  navi.SetShader(pixelated);

  ENGINE.Draw(navi);
}

void SelectNaviScene::onStart()
{
  gotoNextScene = false;
}

void SelectNaviScene::onLeave()
{
}

void SelectNaviScene::onExit()
{
}

void SelectNaviScene::onEnter()
{
}

void SelectNaviScene::onResume()
{
  gotoNextScene = false;
}

void SelectNaviScene::onEnd()
{
}

void SelectNaviScene::onUpdate(double elapsed) {
  this->elapsed = elapsed;

  camera.Update((float)elapsed);
  textbox.Update((float)elapsed);

  glowpadAnimator.Update((float)elapsed, glowpad);

  naviAnimator.Update((float)elapsed, navi);
  bg->Update((float)elapsed);

  SelectedNavi prevSelect = naviSelectionIndex;

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous mob 
        selectInputCooldown = maxSelectInputCooldown;
        naviSelectionIndex = static_cast<SelectedNavi>((int)naviSelectionIndex - 1);

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
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

    if (INPUT.Has(EventTypes::PRESSED_CANCEL)) {
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      textbox.Mute();

      getController().queuePop<swoosh::intent::segue<Checkerboard, swoosh::intent::milli<500>>>();
    }
  }

  naviSelectionIndex = (SelectedNavi)std::max(0, (int)naviSelectionIndex);
  naviSelectionIndex = (SelectedNavi)std::min((int)NAVIS.Size() - 1, (int)naviSelectionIndex);

  if (naviSelectionIndex != prevSelect || !loadNavi) {
    factor = 125;

    naviAnimator = Animation(NAVIS.At(naviSelectionIndex).GetBattleAnimationPath());
    naviAnimator.Reload();
    naviAnimator.SetAnimation("PLAYER_IDLE");
    naviAnimator << Animator::Mode::Loop;

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

  // This just scrambles the letters
  if (numberCooldown > 0) {
    numberCooldown -= (float)elapsed;
    std::string newstr;

    for (int i = 0; i < naviLabel->getString().getSize(); i++) {
      double progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;
      double index = progress * naviLabel->getString().getSize();

      if (i < (int)index) {
        // Choose the unscrambled character from the original string
        newstr += naviLabel->getString()[i];
      }
      else {
        // If the character in the string isn't a space...
        if (naviLabel->getString()[i] != ' ') {
            
          // Choose a random, capital ASCII character
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

  // Darken the unselected navis
  if (prevChosen != naviSelectionIndex) {
    navi.setColor(sf::Color(200, 200, 200, 128));
  }
  else {

    // Make selected navis full color
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
  navi.setPosition(xpos, glowbase.getPosition().y + 10);

  // Make a selection
  if (INPUT.Has(EventTypes::PRESSED_CONFIRM) && prevChosen != naviSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::LOW);
    prevChosen = prevSelect;

    // TODO: Highlight the chosen navi symbol
  }
}
