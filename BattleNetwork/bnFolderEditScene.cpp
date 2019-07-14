#include <time.h>
#include <cmath>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderEditScene.h"
#include "Segues\BlackWashFade.h"
#include "bnChipLibrary.h"
#include "bnChipFolder.h"
#include "Android/bnTouchArea.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

std::string FolderEditScene::FormatChipDesc(const std::string && desc)
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
        output = output.substr(0, index + 1);
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

FolderEditScene::FolderEditScene(swoosh::ActivityController &controller, ChipFolder& folder) :
  camera(sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320))), folder(folder),
  swoosh::Activity(&controller)
{

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("Edit Folder", *font);
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
  //chipDesc->setLineSpacing(5);
  chipDesc->setFillColor(sf::Color::Black);

  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_VIEW_BG));
  bg.setScale(2.f, 2.f);

  folderDock = sf::Sprite(LOAD_TEXTURE(FOLDER_DOCK));
  folderDock.setScale(2.f, 2.f);

  scrollbar = sf::Sprite(LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);

  folderCursor = sf::Sprite(LOAD_TEXTURE(FOLDER_CURSOR));
  folderCursor.setScale(2.f, 2.f);
  folderCursor.setPosition((2.f*90.f), 64.0f);

  packCursor = folderCursor;
  packCursor.setPosition((2.f*90.f) + 480.0f, 64.0f);

  stars = sf::Sprite(LOAD_TEXTURE(FOLDER_RARITY));
  stars.setScale(2.f, 2.f);

  chipHolder = sf::Sprite(LOAD_TEXTURE(FOLDER_CHIP_HOLDER));
  chipHolder.setScale(2.f, 2.f);

  element = sf::Sprite(LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);

  // Current chip graphic
  chip = sf::Sprite(LOAD_TEXTURE(CHIP_CARDS));
  cardSubFrame = sf::IntRect(TEXTURES.GetCardRectFromID(0));
  chip.setTextureRect(cardSubFrame);
  chip.setScale(2.f, 2.f);
  chip.setOrigin(chip.getLocalBounds().width / 2.0f, chip.getLocalBounds().height / 2.0f);

  chipIcon = sf::Sprite(LOAD_TEXTURE(CHIP_ICONS));
  chipIcon.setScale(2.f, 2.f);

  chipRevealTimer.start();
  easeInTimer.start();

  /* foldet view */
  folderView.maxChipsOnScreen = 7;
  folderView.currChipIndex = folderView.lastChipOnScreen = folderView.prevIndex = 0;
  folderView.numOfChips = folder.GetSize();

  /* library view */
  packView.maxChipsOnScreen = 7;
  packView.currChipIndex = packView.lastChipOnScreen = packView.prevIndex = 0;
  packView.numOfChips = CHIPLIB.GetSize();

  prevViewMode = currViewMode = ViewMode::FOLDER;

  totalTimeElapsed = frameElapsed = 0.0;

  canInteract = false;
}

FolderEditScene::~FolderEditScene() { ; }

void FolderEditScene::onStart() {
  ENGINE.SetCamera(camera);

  canInteract = true;
}

void FolderEditScene::onUpdate(double elapsed) {
  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  auto offset = camera.GetView().getCenter().x - 240;
  bg.setPosition(offset, 0.f);
  menuLabel->setPosition(offset + 20.0f, 5.f);

  camera.Update((float)elapsed);
  this->setView(camera.GetView());

  // Scene keyboard controls
  if (canInteract) {
    ChipView* view = nullptr;

    if (currViewMode == ViewMode::FOLDER) {
      view = &folderView;
    }
    else if (currViewMode == ViewMode::PACK) {
      view = &packView;
    }

    if (INPUT.Has(PRESSED_UP)) {
      selectInputCooldown -= elapsed;

      view->prevIndex = view->currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        view->currChipIndex--;
        AUDIO.Play(AudioType::CHIP_SELECT);

        if (view->currChipIndex < view->lastChipOnScreen) {
          --view->lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else if (INPUT.Has(PRESSED_DOWN)) {
      selectInputCooldown -= elapsed;

      view->prevIndex = view->currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        view->currChipIndex++;
        AUDIO.Play(AudioType::CHIP_SELECT);

        if (view->currChipIndex > view->lastChipOnScreen + view->maxChipsOnScreen - 1) {
          ++view->lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (INPUT.Has(PRESSED_RIGHT) && currViewMode == ViewMode::FOLDER) {
      currViewMode = ViewMode::PACK;
      canInteract = false;
    }
    else if (INPUT.Has(PRESSED_LEFT) && currViewMode == ViewMode::PACK) {
      currViewMode = ViewMode::FOLDER;
      canInteract = false;
    }

    view->currChipIndex = std::max(0, view->currChipIndex);
    view->currChipIndex = std::min(view->numOfChips - 1, view->currChipIndex);

    view->lastChipOnScreen = std::max(0, view->lastChipOnScreen);
    view->lastChipOnScreen = std::min(view->numOfChips - 1, view->lastChipOnScreen);

    if (INPUT.Has(PRESSED_B) && canInteract) {
      canInteract = false;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

      using swoosh::intent::direction;
      using segue = swoosh::intent::segue<BlackWashFade>;
      getController().queuePop<segue>();
    }
  }
  else {
    if (prevViewMode != currViewMode) {
      if (currViewMode == ViewMode::FOLDER) {
        if (camera.GetView().getCenter().x > 240) {
          camera.OffsetCamera(sf::Vector2f(-480.0f * elapsed, 0));
        }
        else {
          prevViewMode = currViewMode;
          canInteract = true;
        }
      }
      else if (currViewMode == ViewMode::PACK) {
        if (camera.GetView().getCenter().x < 720) {
          camera.OffsetCamera(sf::Vector2f(480.0f * elapsed, 0));
        }
        else {
          prevViewMode = currViewMode;
          canInteract = true;
        }
      }
      else {
        prevViewMode = currViewMode;
        canInteract = true;
      }
    }
  }
}

void FolderEditScene::onLeave() {

}

void FolderEditScene::onExit()
{
}

void FolderEditScene::onEnter()
{
}

void FolderEditScene::onResume() {

}

void FolderEditScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  DrawFolder();
  DrawLibrary();
}

void FolderEditScene::DrawFolder() {
  chipDesc->setPosition(sf::Vector2f(20.f, 185.0f));
  folderDock.setPosition(2.f, 30.f);
  scrollbar.setPosition(410.f, 60.f);
  chipHolder.setPosition(4.f, 35.f);
  element.setPosition(2.f*25.f, 146.f);
  chip.setPosition(83.f, 93.f);

  folderDock.setScale(2.0f, 2.0f);

  ENGINE.Draw(folderDock);
  ENGINE.Draw(chipHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)folderView.lastChipOnScreen / (float)folderView.numOfChips)*bottom;
  scrollbar.setPosition(452.f, top + depth);

  ENGINE.Draw(scrollbar);

  Logger::Log(std::string("folder view: ") + std::to_string(folderView.numOfChips));
  Logger::Log(std::string("folder: ") + std::to_string(folder.GetSize()));

  if (folder.GetSize() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  ChipFolder::Iter iter = folder.Begin();

  for (int j = 0; j < folderView.lastChipOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < folderView.maxChipsOnScreen && folderView.lastChipOnScreen + i < folderView.numOfChips; i++) {
    chipIcon.setTextureRect(TEXTURES.GetIconRectFromID((*iter)->GetIconID()));
    chipIcon.setPosition(2.f*104.f, 65.0f + (32.f*i));
    ENGINE.Draw(chipIcon, false);

    chipLabel->setFillColor(sf::Color::White);
    chipLabel->setPosition(2.f*120.f, 60.0f + (32.f*i));
    chipLabel->setString((*iter)->GetShortName());
    ENGINE.Draw(chipLabel, false);


    int offset = (int)((*iter)->GetElement());
    element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
    element.setPosition(2.f*173.f, 65.0f + (32.f*i));
    ENGINE.Draw(element, false);

    chipLabel->setOrigin(0, 0);
    chipLabel->setPosition(2.f*190.f, 60.0f + (32.f*i));
    chipLabel->setString(std::string() + (*iter)->GetCode());
    ENGINE.Draw(chipLabel, false);

    //Draw rating
    unsigned rarity = (*iter)->GetRarity() - 1;
    stars.setTextureRect(sf::IntRect(0, 15 * rarity, 22, 14));
    stars.setPosition(2.f*199.f, 74.0f + (32.f*i));
    ENGINE.Draw(stars, false);

    // Draw cursor
    if (folderView.lastChipOnScreen + i == folderView.currChipIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, folderCursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*5.0f;

      folderCursor.setPosition((2.f*90.f) + bounce, y);
      ENGINE.Draw(folderCursor);

      sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID((*iter)->GetID());
      chip.setTextureRect(cardSubFrame);
      chip.setScale((float)swoosh::ease::linear(chipRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
      ENGINE.Draw(chip, false);

      // This draws the currently highlighted chip
      if ((*iter)->GetDamage() > 0) {
        chipLabel->setFillColor(sf::Color::White);
        chipLabel->setString(std::to_string((*iter)->GetDamage()));
        chipLabel->setOrigin(chipLabel->getLocalBounds().width + chipLabel->getLocalBounds().left, 0);
        chipLabel->setPosition(2.f*(70.f), 135.f);

        ENGINE.Draw(chipLabel, false);
      }

      chipLabel->setOrigin(0, 0);
      chipLabel->setFillColor(sf::Color::Yellow);
      chipLabel->setPosition(2.f*14.f, 135.f);
      chipLabel->setString(std::string() + (*iter)->GetCode());
      ENGINE.Draw(chipLabel, false);

      std::string formatted = FormatChipDesc((*iter)->GetDescription());
      chipDesc->setString(formatted);
      ENGINE.Draw(chipDesc, false);

      int offset = (int)((*iter)->GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*25.f, 142.f);
      ENGINE.Draw(element, false);
    }

    iter++;
  }
}

void FolderEditScene::DrawLibrary() {
  chipDesc->setPosition(sf::Vector2f(20.f + 480.f, 185.0f));
  folderDock.setScale(-2.0f, 2.0f);

  folderDock.setPosition((2.f + 480.f)*1.5f, 30.f);
  scrollbar.setPosition(410.f + 480.f, 60.f);
  chipHolder.setPosition(4.f + 480.f, 35.f);
  element.setPosition(2.f*25.f + 480.f, 146.f);
  chip.setPosition(83.f + 480.f, 93.f);

  ENGINE.Draw(folderDock);
  ENGINE.Draw(chipHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)packView.lastChipOnScreen / (float)packView.numOfChips)*bottom;
  scrollbar.setPosition(452.f + 480.f, top + depth);

  ENGINE.Draw(scrollbar);

  if (CHIPLIB.GetSize() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  ChipLibrary::Iter iter = CHIPLIB.Begin();

  for (int j = 0; j < packView.lastChipOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < packView.maxChipsOnScreen && packView.lastChipOnScreen + i < packView.numOfChips; i++) {
    chipIcon.setTextureRect(TEXTURES.GetIconRectFromID((*iter).GetIconID()));
    chipIcon.setPosition(2.f*104.f + 480.f, 65.0f + (32.f*i));
    ENGINE.Draw(chipIcon, false);

    chipLabel->setFillColor(sf::Color::White);
    chipLabel->setPosition(2.f*120.f + 480.f, 60.0f + (32.f*i));
    chipLabel->setString((*iter).GetShortName());
    ENGINE.Draw(chipLabel, false);


    int offset = (int)((*iter). GetElement());
    element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
    element.setPosition(2.f*173.f + 480.f, 65.0f + (32.f*i));
    ENGINE.Draw(element, false);

    chipLabel->setOrigin(0, 0);
    chipLabel->setPosition(2.f*190.f + 480.f, 60.0f + (32.f*i));
    chipLabel->setString(std::string() + (*iter).GetCode());
    ENGINE.Draw(chipLabel, false);

    //Draw rating
    unsigned rarity = (*iter).GetRarity() - 1;
    stars.setTextureRect(sf::IntRect(0, 15 * rarity, 22, 14));
    stars.setPosition(2.f*199.f + 480.f, 74.0f + (32.f*i));
    ENGINE.Draw(stars, false);

    // Draw cursor
    if (packView.lastChipOnScreen + i == packView.currChipIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, packCursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*5.0f;

      packCursor.setPosition((2.f*90.f) + bounce + 480.f, y);
      ENGINE.Draw(packCursor);

      sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID((*iter).GetID());
      chip.setTextureRect(cardSubFrame);
      chip.setScale((float)swoosh::ease::linear(chipRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
      ENGINE.Draw(chip, false);

      // This draws the currently highlighted chip
      if ((*iter).GetDamage() > 0) {
        chipLabel->setFillColor(sf::Color::White);
        chipLabel->setString(std::to_string((*iter).GetDamage()));
        chipLabel->setOrigin(chipLabel->getLocalBounds().width + chipLabel->getLocalBounds().left, 0);
        chipLabel->setPosition(2.f*(70.f) + 480.f, 135.f);

        ENGINE.Draw(chipLabel, false);
      }

      chipLabel->setOrigin(0, 0);
      chipLabel->setFillColor(sf::Color::Yellow);
      chipLabel->setPosition(2.f*14.f + 480.f, 135.f);
      chipLabel->setString(std::string() + (*iter).GetCode());
      ENGINE.Draw(chipLabel, false);

      std::string formatted = FormatChipDesc((*iter).GetDescription());
      chipDesc->setString(formatted);
      ENGINE.Draw(chipDesc, false);

      int offset = (int)((*iter).GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*25.f + 480.f, 142.f);
      ENGINE.Draw(element, false);
    }

    iter++;
  }
}

void FolderEditScene::onEnd() {
  delete font;
  delete chipFont;
  delete chipDescFont;
  delete numberFont;
  delete menuLabel;
  delete numberLabel;
  delete chipDesc;
}


#ifdef __ANDROID__
void FolderEditScene::StartupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);

  rightSide.onTouch([]() {
      INPUT.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([this](sf::Vector2i delta) {
      if(!this->releasedB) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_A);
      }
  });

  rightSide.onDrag([this](sf::Vector2i delta){
      if(delta.x < -25 && !this->releasedB) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_B);
        INPUT.VirtualKeyEvent(InputEvent::RELEASED_B);
        this->releasedB = true;
      }
  });
}

void FolderEditScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif