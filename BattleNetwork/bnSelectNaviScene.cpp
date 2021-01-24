
#include <Swoosh/ActivityController.h>
#include <Swoosh/Game.h>

#include "bnSelectNaviScene.h"
#include "Segues/Checkerboard.h"

using namespace swoosh::types;

SelectNaviScene::SelectNaviScene(swoosh::ActivityController& controller, SelectedNavi& currentNavi) :
  naviSelectionIndex(currentNavi),
  currentChosen(currentNavi),
  font(Font::Style::small),
  textbox(135, 15, font),
  naviFont(Font::Style::thick),
  naviLabel("No Data", naviFont),
  hpLabel("1", font),
  attackLabel("1", font),
  speedLabel("1", font),
  menuLabel("1", font),
  Scene(controller) {

  // Menu name font
  menuLabel.setPosition(sf::Vector2f(20.f, 5.0f));
  menuLabel.setScale(2.f, 2.f);

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // NAVI UI font
  naviLabel.setPosition(30.f, 22.f);
  naviLabel.setScale(2.0f, 2.0f);

  attackLabel.setPosition(335.f, 24.f);
  attackLabel.setScale(2.0f, 2.0f);

  speedLabel.setPosition(335.f, 78.f);
  speedLabel.setScale(2.0f, 2.0f);

  hpLabel.setPosition(335.f, 132.0f);
  hpLabel.setScale(2.0f, 2.0f);

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

  element.setTexture(Textures().GetTexture(TextureType::ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(UI_LEFT_POS_MAX + 15.f, 90);

  // Current navi graphic
  loadNavi = false;
  navi.setScale(2.f, 2.f);
  navi.setOrigin(navi.getLocalBounds().width / 2.f, navi.getLocalBounds().height / 2.f);
  navi.setPosition(100.f, 150.f);

  navi.setTexture(NAVIS.At(currentChosen).GetPreviewTexture());

  naviLabel.SetString(sf::String(NAVIS.At(currentChosen).GetName().c_str()));
  speedLabel.SetString(sf::String(NAVIS.At(currentChosen).GetSpeedString().c_str()));
  attackLabel.SetString(sf::String(NAVIS.At(currentChosen).GetAttackString().c_str()));
  hpLabel.SetString(sf::String(NAVIS.At(currentChosen).GetHPString().c_str()));
  
  // Distortion effect
  factor = MAX_PIXEL_FACTOR;

  gotoNextScene = true;

  // Text box
  textbox.SetCharactersPerSecond(15);
  textbox.setPosition(UI_RIGHT_POS_MAX + 10, 205);
  textbox.Stop();
  // textbox.
  textbox.Mute(); // no tick sound

  elapsed = 0;

  setView(sf::Vector2u(480, 320));
}

SelectNaviScene::~SelectNaviScene()
{
  delete bg;
}

void SelectNaviScene::onDraw(sf::RenderTexture& surface) {
  surface.draw(*bg);

  // Navi preview shadow
  auto originalPosition = navi.getPosition();
  auto originalColor = navi.getColor();

  // Make the shadow begin on the other side of the window by an arbitrary offset
  navi.setPosition(-20.0f + getController().getVirtualWindowSize().x - navi.getPosition().x, navi.getPosition().y);
  navi.setColor(sf::Color::Black);
  surface.draw(navi);

  // End 'hack' by restoring original position and color values
  navi.setPosition(originalPosition);
  navi.setColor(originalColor);

  charName.setPosition(UI_LEFT_POS, charName.getPosition().y);
  surface.draw(charName);

  charElement.setPosition(UI_LEFT_POS, charElement.getPosition().y);
  surface.draw(charElement);

  // Draw stat box three times for three diff. properties
  float charStat1Max = 10;

  if (UI_TOP_POS < charStat1Max) {
    charStat.setPosition(UI_RIGHT_POS, charStat1Max);
  }
  else {
    charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
  }

  surface.draw(charStat);

  // 2nd stat box
  float charStat2Max = 10 + UI_SPACING;

  if (UI_TOP_POS < charStat2Max) {
    charStat.setPosition(UI_RIGHT_POS, charStat2Max);
  }
  else {
    charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
  }

  surface.draw(charStat);

  // 3rd stat box
  float charStat3Max = 10 + (UI_SPACING * 2);

  if (UI_TOP_POS < charStat3Max) {
    charStat.setPosition(UI_RIGHT_POS, charStat3Max);
  }
  else {
    charStat.setPosition(UI_RIGHT_POS, UI_TOP_POS);
  }

  surface.draw(charStat);

  // SP. Info box
  charInfo.setPosition(UI_RIGHT_POS, charInfo.getPosition().y);
  surface.draw(charInfo);

  // Update UI slide in
  if (!gotoNextScene) {
    factor -= (float)elapsed * 280.f;

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
        surface.draw(naviLabel);
        surface.draw(hpLabel);
        surface.draw(speedLabel);
        surface.draw(attackLabel);
        surface.draw(textbox);
        surface.draw(element);

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

  surface.draw(navi);
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
  SelectNaviScene::elapsed = elapsed;

  textbox.Update((float)elapsed);

  bg->Update((float)elapsed);

  SelectedNavi prevSelect = currentChosen;

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (Input().Has(InputEvents::pressed_ui_left)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous mob
        selectInputCooldown = maxSelectInputCooldown;
        currentChosen = static_cast<SelectedNavi>((int)currentChosen - 1);

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else if (Input().Has(InputEvents::pressed_ui_right)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to next mob
        selectInputCooldown = maxSelectInputCooldown;
        currentChosen = static_cast<SelectedNavi>((int)currentChosen + 1);

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (Input().Has(InputEvents::pressed_cancel)) {
      gotoNextScene = true;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      textbox.Mute();

      getController().pop<segue<Checkerboard, milliseconds<500>>>();
    }
  }

  currentChosen = (SelectedNavi)std::max(0, (int)currentChosen);
  currentChosen = (SelectedNavi)std::min((int)NAVIS.Size() - 1, (int)currentChosen);

  // Reset the factor/slide in effects if a new selection was made
  if (currentChosen != prevSelect || !loadNavi) {
    factor = 125;

    int offset = (int)(NAVIS.At(currentChosen).GetElement());
    auto iconRect = sf::IntRect(14 * offset, 0, 14, 14);
    element.setTextureRect(iconRect);

    navi.setTexture(NAVIS.At(currentChosen).GetPreviewTexture(), true);
    textbox.SetText(NAVIS.At(currentChosen).GetSpecialDescriptionString());
    loadNavi = true;
  }

  // This goes here because the jumbling effect may finish and we need to see proper values
  naviLabel.SetString(sf::String(NAVIS.At(currentChosen).GetName()));
  speedLabel.SetString(sf::String(NAVIS.At(currentChosen).GetSpeedString()));
  attackLabel.SetString(sf::String(NAVIS.At(currentChosen).GetAttackString()));
  hpLabel.SetString(sf::String(NAVIS.At(currentChosen).GetHPString()));

  // This just scrambles the letters
  if (numberCooldown > 0) {
    numberCooldown -= (float)elapsed;
    std::string newstr;

    for (int i = 0; i < naviLabel.GetString().length(); i++) {
      double progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;
      double index = progress * naviLabel.GetString().length();

      if (i < (int)index) {
        // Choose the unscrambled character from the original string
        newstr += naviLabel.GetString()[i];
      }
      else {
        // If the character in the string isn't a space...
        if (naviLabel.GetString()[i] != ' ') {
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

    //attackLabel.SetString(std::to_string(randAttack));
    //speedLabel.SetString(std::to_string(randSpeed));
    naviLabel.SetString(sf::String(newstr));
  }

  float progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;

  if (progress > 1.f) progress = 1.f;

  // Darken the unselected navis
  if (prevChosen != currentChosen) {
    navi.setColor(sf::Color(200, 200, 200, 188));
  }
  else {

    // Make selected navis full color
    navi.setColor(sf::Color(255, 255, 255, 255));
  }

  // transform this value into a 0 -> 1 range
  float range = (125.f - (float)factor) / 125.f;

  if (factor != 0.f) {
    navi.setColor(sf::Color(255, 255, 255, (sf::Uint8)(navi.getColor().a * range)));
  }

  // Refresh mob graphic origin every frame as it may change
  auto size = getController().getVirtualWindowSize();

  navi.setPosition(range*float(size.x)*0.425f, float(size.y));
  navi.setOrigin(float(navi.getTextureRect().width)*0.5f, float(navi.getTextureRect().height));

  // Make a selection
  if (Input().Has(InputEvents::pressed_confirm) && currentChosen != naviSelectionIndex) {
    Audio().Play(AudioType::CHIP_CONFIRM, AudioPriority::low);
    prevChosen = currentChosen;
    naviSelectionIndex = currentChosen;
    WEBCLIENT.SetKey("SelectedNavi", std::to_string(naviSelectionIndex));
  }
}
