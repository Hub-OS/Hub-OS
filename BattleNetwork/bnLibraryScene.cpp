#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnLibraryScene.h"
#include "bnChipLibrary.h"
#include "bnChipFolder.h"

#include <SFML/Graphics.hpp>
#include <cmath>

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#include "Segues/PushIn.h"

std::string LibraryScene::FormatChipDesc(const std::string && desc)
{
  std::string output = desc;

  auto bothAreSpaces = [](char lhs, char rhs) -> bool { return (lhs == rhs) && (lhs == ' '); };

  std::string::iterator new_end = std::unique(output.begin(), output.end(), bothAreSpaces);
  output.erase(new_end, output.end());

  int index = 0;
  int perLine = 0;
  int line = 0;
  int wordIndex = 0; // If we are breaking on a word

  bool finished = false;

  while (index != output.size() && !finished) {
    if (output[index] != ' ' && wordIndex == -1) {
      wordIndex = index;
    }
    else if (output[index] == ' ' && wordIndex > -1) {
      wordIndex = -1;
    }

    if (perLine > 0 && perLine % 9 == 0) {
      if (wordIndex > -1) {
        if (index == wordIndex + 9) {
          wordIndex = index; // We have no choice but to cut off part of this lengthy word
        }
        // Line break at the last word
        //while (desc[index] == ' ') { index++; }
        output.insert(wordIndex, "\n");
        index = wordIndex; // Start counting from here
        while (output[index] == ' ') { output.erase(index); }
      }
      else {
        // Line break at the next word
        while (output[index] == ' ') { index++; }
        output.insert(index, "\n"); index++;
        while (output[index] == ' ') { output.erase(index); }
      }
      line++;

      if (line == 3) {
        output = output.substr(0, index+1);
        output[output.size() - 1] = ';'; // font glyph will show an elipses
        finished = true;
      }

      perLine = 0;
      wordIndex = -1;
    }
    
    perLine++;
    index++;
  }

  // Chip docks can only fit 9 characters per line, 3 lines total
  if (output.size() > 3 * 10) {
    output = output.substr(0, (3 * 10));
    output[output.size() - 1] = ';'; // font glyph will show an elipses
  }

  return output;
}

LibraryScene::LibraryScene(swoosh::ActivityController &controller) :
  camera(ENGINE.GetDefaultView()),
  textbox(sf::Vector2f(4, 255)),
  swoosh::Activity(&controller)
{

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("Library", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // Chip UI font
  chipFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  chipLabel = new sf::Text("", *chipFont);
  chipLabel->setPosition(275.f, 15.f);

  numberFont = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  numberLabel = new sf::Text("", *numberFont);
  numberLabel->setOutlineColor(sf::Color(48, 56, 80));
  numberLabel->setOutlineThickness(2.f);
  numberLabel->setScale(0.8f, 0.8f);
  numberLabel->setOrigin(numberLabel->getLocalBounds().width, 0);
  numberLabel->setPosition(sf::Vector2f(170.f, 28.0f));

  // Chip description font
  chipDescFont = TEXTURES.LoadFontFromFile("resources/fonts/NETNAVI_4-6_V3.ttf");
  chipDesc = new sf::Text("", *chipDescFont);
  chipDesc->setCharacterSize(24);
  chipDesc->setPosition(sf::Vector2f(20.f, 185.0f));
  //chipDesc->setLineSpacing(5);
  chipDesc->setFillColor(sf::Color::Black);

  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_VIEW_BG));
  bg.setScale(2.f, 2.f);

  folderDock = sf::Sprite(LOAD_TEXTURE(FOLDER_DOCK));
  folderDock.setScale(2.f, 2.f);
  folderDock.setPosition(2.f, 30.f);

  scrollbar = sf::Sprite(LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);
  scrollbar.setPosition(410.f, 60.f);

  cursor = sf::Sprite(LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);
  cursor.setPosition((2.f*90.f), 64.0f);

  stars = sf::Sprite(LOAD_TEXTURE(FOLDER_RARITY));
  stars.setScale(2.f, 2.f);

  chipHolder = sf::Sprite(LOAD_TEXTURE(FOLDER_CHIP_HOLDER));
  chipHolder.setScale(2.f, 2.f);
  chipHolder.setPosition(4.f, 35.f);

  element = sf::Sprite(LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  // Current chip graphic
  chip = sf::Sprite(LOAD_TEXTURE(CHIP_CARDS));
  cardSubFrame = sf::IntRect(TEXTURES.GetCardRectFromID(0));
  chip.setTextureRect(cardSubFrame);
  chip.setScale(2.f, 2.f);
  chip.setPosition(83.f, 93.f);
  chip.setOrigin(chip.getLocalBounds().width / 2.0f, chip.getLocalBounds().height / 2.0f);

  chipIcon = sf::Sprite(LOAD_TEXTURE(CHIP_ICONS));
  chipIcon.setScale(2.f, 2.f);

  chipRevealTimer.start();
  easeInTimer.start();

  maxChipsOnScreen = 7;
  currChipIndex = lastChipOnScreen = prevIndex = 0;
  totalTimeElapsed = frameElapsed = 0.0;

  this->MakeUniqueChipsFromPack();
}

LibraryScene::~LibraryScene() { ; }

void LibraryScene::MakeUniqueChipsFromPack()
{
  ChipLibrary::Iter iter = CHIPLIB.Begin();

  for (iter; iter != CHIPLIB.End(); iter++) {
    this->uniqueChips.insert(uniqueChips.begin(), *iter);
  }

  auto pred = [](const Chip &a, const Chip &b) -> bool {
    return a.GetShortName() == b.GetShortName();
  };

  this->uniqueChips.unique(pred);

  numOfChips = (int)uniqueChips.size();
}

void LibraryScene::onStart() {
  ENGINE.SetCamera(camera);

  gotoNextScene = false;
}

void LibraryScene::onUpdate(double elapsed) {
  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  camera.Update((float)elapsed);
  textbox.Update((float)elapsed);

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUT.has(PRESSED_UP)) {
      selectInputCooldown -= elapsed;

      prevIndex = currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currChipIndex--;
        AUDIO.Play(AudioType::CHIP_SELECT);

        if (currChipIndex < lastChipOnScreen) {
          --lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else if (INPUT.has(PRESSED_DOWN)) {
      selectInputCooldown -= elapsed;

      prevIndex = currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currChipIndex++;
        AUDIO.Play(AudioType::CHIP_SELECT);

        if (currChipIndex > lastChipOnScreen + maxChipsOnScreen - 1) {
          ++lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (INPUT.has(PRESSED_A) && textbox.IsClosed()) {
      auto iter = uniqueChips.begin();
      int i = 0;

      while (i < currChipIndex) {
        i++;
        iter++;
      }

      textbox.DequeMessage(); // make sure textbox is empty
      textbox.EnqueMessage(sf::Sprite(), "", new AnimatedTextBox::Message(iter->GetVerboseDescription()));
      textbox.Open();
      AUDIO.Play(AudioType::CHIP_DESC);
    }
    else if (INPUT.has(RELEASED_B) && textbox.IsOpen()) {
      textbox.Close();
      textbox.SetTextSpeed(1.0);
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
    }
    else if (INPUT.has(PRESSED_A) && textbox.IsOpen()) {
      textbox.SetTextSpeed(3.0);
    }
    else if (INPUT.has(RELEASED_A) && textbox.IsOpen()) {
      textbox.SetTextSpeed(1.0);
      textbox.Continue();
    }

    currChipIndex = std::max(0, currChipIndex);
    currChipIndex = std::min(numOfChips - 1, currChipIndex);

    lastChipOnScreen = std::max(0, lastChipOnScreen);
    lastChipOnScreen = std::min(numOfChips - 1, lastChipOnScreen);

    if (INPUT.has(PRESSED_B) && textbox.IsClosed()) {
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

      using swoosh::intent::direction;
      using segue = swoosh::intent::segue<PushIn<direction::left>>;
      getController().queuePop<segue>();
    }
  }
}

void LibraryScene::onLeave() {

}

void LibraryScene::onExit()
{
}

void LibraryScene::onEnter()
{
}

void LibraryScene::onResume() {

}

void LibraryScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  ENGINE.DrawUnderlay();
  ENGINE.DrawLayers();
  ENGINE.DrawOverlay();

  ENGINE.Draw(folderDock);
  ENGINE.Draw(chipHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)lastChipOnScreen / (float)numOfChips)*bottom;
  scrollbar.setPosition(452.f, top + depth);

  ENGINE.Draw(scrollbar);

  if (uniqueChips.size() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  ChipLibrary::Iter iter = uniqueChips.begin();

  for (int j = 0; j < lastChipOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < maxChipsOnScreen && lastChipOnScreen + i < numOfChips; i++) {
    chipIcon.setTextureRect(TEXTURES.GetIconRectFromID(iter->GetIconID()));
    chipIcon.setPosition(2.f*104.f, 65.0f + (32.f*i));
    ENGINE.Draw(chipIcon, false);

    chipLabel->setOrigin(0, 0);
    chipLabel->setFillColor(sf::Color::White);
    chipLabel->setPosition(2.f*120.f, 60.0f + (32.f*i));
    chipLabel->setString(iter->GetShortName());
    ENGINE.Draw(chipLabel, false);

    //Draw rating
    unsigned rarity = iter->GetRarity() - 1;
    stars.setTextureRect(sf::IntRect(0, 16 * rarity, 22, 16));
    stars.setPosition(2.f*199.f, 74.0f + (32.f*(float)i));
    ENGINE.Draw(stars, false);

    // Draw cursor
    if (lastChipOnScreen + i == currChipIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, cursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*5.0f;

      cursor.setPosition((2.f*90.f) + bounce, y);
      ENGINE.Draw(cursor);

      sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(iter->GetID());
      chip.setTextureRect(cardSubFrame);
      chip.setScale((float)swoosh::ease::linear(chipRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
      ENGINE.Draw(chip, false);

      // This draws the currently highlighted chip
      if (iter->GetDamage() > 0) {
        chipLabel->setFillColor(sf::Color::White);
        chipLabel->setString(std::to_string(iter->GetDamage()));
        chipLabel->setOrigin(chipLabel->getLocalBounds().width + chipLabel->getLocalBounds().left, 0);
        chipLabel->setPosition(2.f*(70.f), 135.f);

        ENGINE.Draw(chipLabel, false);
      }

      std::string formatted = FormatChipDesc(iter->GetDescription());
      chipDesc->setString(formatted);
      ENGINE.Draw(chipDesc, false);

      int offset = (int)(iter->GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*25.f, 142.f);
      ENGINE.Draw(element, false);
    }

    iter++;
  }

  ENGINE.Draw(textbox);
}

void LibraryScene::onEnd() {
  delete font;
  delete chipFont;
  delete chipDescFont;
  delete numberFont;
  delete menuLabel;
  delete numberLabel;
  delete chipDesc;
}
