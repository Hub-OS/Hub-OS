#include <time.h>
#include <cmath>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderEditScene.h"
#include "Segues/BlackWashFade.h"
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
    output[output.size()] = ';'; // font glyph will show an elipses
  }

  return output;
}

FolderEditScene::FolderEditScene(swoosh::ActivityController &controller, ChipFolder& folder) :
  camera(sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320))), folder(folder), hasFolderChanged(false),
  swoosh::Activity(&controller)
{
  // Move chip data into their appropriate containers for easier management
  PlaceFolderDataIntoChipSlots();
  PlaceLibraryDataIntoBuckets();

  // We must account for existing chip data to accurately represent what's left from our pool
  ExcludeFolderDataFromPack();

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("Folder Edit", *font);
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
  chipDesc->setFillColor(sf::Color::Black);

  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_VIEW_BG));
  bg.setScale(2.f, 2.f);

  folderDock = sf::Sprite(LOAD_TEXTURE(FOLDER_DOCK));
  folderDock.setScale(2.f, 2.f);
  folderDock.setPosition(2.f, 30.f);

  packDock = sf::Sprite(LOAD_TEXTURE(PACK_DOCK));
  packDock.setScale(2.f, 2.f);
  packDock.setPosition(480.f, 30.f);

  scrollbar = sf::Sprite(LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);

  folderCursor = sf::Sprite(LOAD_TEXTURE(FOLDER_CURSOR));
  folderCursor.setScale(2.f, 2.f);
  folderCursor.setPosition((2.f*90.f), 64.0f);
  folderSwapCursor = folderCursor;

  packCursor = folderCursor;
  packCursor.setPosition((2.f*90.f) + 480.0f, 64.0f);
  packSwapCursor = packCursor;

  mbPlaceholder = sf::Sprite(LOAD_TEXTURE(FOLDER_MB));
  mbPlaceholder.setScale(2.f, 2.f);

  folderNextArrow = sf::Sprite(LOAD_TEXTURE(FOLDER_NEXT_ARROW));
  folderNextArrow.setScale(2.f, 2.f);

  packNextArrow = folderNextArrow;
  packNextArrow.setScale(-2.f, 2.f);

  folderChipCountBox = sf::Sprite(LOAD_TEXTURE(FOLDER_SIZE));
  folderChipCountBox.setPosition(sf::Vector2f(425.f, 10.f + folderChipCountBox.getLocalBounds().height));
  folderChipCountBox.setScale(2.f, 2.f);
  folderChipCountBox.setOrigin(folderChipCountBox.getLocalBounds().width / 2.0f, folderChipCountBox.getLocalBounds().height / 2.0f);

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
  folderView.swapChipIndex = -1;
  folderView.numOfChips = int(folderChipSlots.size());

  /* library view */
  packView.maxChipsOnScreen = 7;
  packView.currChipIndex = packView.lastChipOnScreen = packView.prevIndex = 0;
  packView.swapChipIndex = -1;
  packView.numOfChips = int(packChipBuckets.size());

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

    if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
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
    else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
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
    }else if (INPUT.Has(EventTypes::PRESSED_SCAN_LEFT)) {
      selectInputCooldown -= elapsed;

      view->prevIndex = view->currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        view->currChipIndex -= view->maxChipsOnScreen;

        view->currChipIndex = std::max(view->currChipIndex, 0);

        AUDIO.Play(AudioType::CHIP_SELECT);

        while (view->currChipIndex < view->lastChipOnScreen) {
          --view->lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_SCAN_RIGHT)) {
      selectInputCooldown -= elapsed;

      view->prevIndex = view->currChipIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        view->currChipIndex += view->maxChipsOnScreen;
        AUDIO.Play(AudioType::CHIP_SELECT);

        view->currChipIndex = std::min(view->currChipIndex, view->numOfChips-1);

        while (view->currChipIndex > view->lastChipOnScreen + view->maxChipsOnScreen - 1) {
          ++view->lastChipOnScreen;
        }

        chipRevealTimer.reset();
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
      if (currViewMode == ViewMode::FOLDER) {
        if (folderView.swapChipIndex != -1) {
          if (folderView.swapChipIndex == folderView.currChipIndex) {
            // Unselect the chip
            folderView.swapChipIndex = -1;
            AUDIO.Play(AudioType::CHIP_CANCEL);

          }
          else {
            // swap the chip
            auto temp = folderChipSlots[folderView.swapChipIndex];
            folderChipSlots[folderView.swapChipIndex] = folderChipSlots[folderView.currChipIndex];
            folderChipSlots[folderView.currChipIndex] = temp;
            AUDIO.Play(AudioType::CHIP_CONFIRM);

            folderView.swapChipIndex = -1;
          }
        }
        else {
          // See if we're swapping from a pack
          if (packView.swapChipIndex != -1) {
            // Try to swap the chip with the one from the pack
            // The chip from the pack is copied and added to the slot
            // The slot chip needs to find its corresponding bucket and increment it
            Chip copy;

            bool gotChip = false;

            // If the pack pointed to is the same as the chip in our folder, add the chip back into folder
            if (packChipBuckets[packView.swapChipIndex].ViewChip() == folderChipSlots[folderView.currChipIndex].ViewChip()) {
              packChipBuckets[packView.swapChipIndex].AddChip();
              folderChipSlots[folderView.currChipIndex].GetChip(copy);

              gotChip = true;
            }
            else if (packChipBuckets[packView.swapChipIndex].GetChip(copy)) {
              Chip prev;

              bool findBucket = folderChipSlots[folderView.currChipIndex].GetChip(prev);

              folderChipSlots[folderView.currChipIndex].AddChip(copy);

              // If the chip slot had a chip, find the corresponding bucket to add it back into
              if (findBucket) {
                auto iter = std::find_if(packChipBuckets.begin(), packChipBuckets.end(),
                  [&prev](const PackBucket& in) { return prev.GetShortName() == in.ViewChip().GetShortName(); }
                );

                if (iter != packChipBuckets.end()) {
                  iter->AddChip();
                }
              }

              gotChip = true;
            }

            if (gotChip) {
              hasFolderChanged = true;

              packView.swapChipIndex = -1;
              folderView.swapChipIndex = -1;

              AUDIO.Play(AudioType::CHIP_CONFIRM);
            }
            else {
              AUDIO.Play(AudioType::CHIP_ERROR);
            }
          }
          else {
            // select the chip
            folderView.swapChipIndex = folderView.currChipIndex;
            AUDIO.Play(AudioType::CHIP_CHOOSE);
          }
        }
      }
      else if (currViewMode == ViewMode::PACK) {
        if (packView.swapChipIndex != -1) {
          if (packView.swapChipIndex == packView.currChipIndex) {
            // Unselect the chip
            packView.swapChipIndex = -1;
            AUDIO.Play(AudioType::CHIP_CANCEL);

          }
          else {
            // swap the pack
            auto temp = packChipBuckets[packView.swapChipIndex];
            packChipBuckets[packView.swapChipIndex] = packChipBuckets[packView.currChipIndex];
            packChipBuckets[packView.currChipIndex] = temp;
            AUDIO.Play(AudioType::CHIP_CONFIRM);

            packView.swapChipIndex = -1;
          }
        }
        else {
          // See if we're swapping from our folder
          if (folderView.swapChipIndex != -1) {
            // Try to swap the chip with the one from the folder
            // The chip from the pack is copied and added to the slot
            // The slot chip needs to find its corresponding bucket and increment it
            Chip copy;

            bool gotChip = false;

            // If the pack pointed to is the same as the chip in our folder, add the chip back into folder
            if (packChipBuckets[packView.currChipIndex].ViewChip() == folderChipSlots[folderView.swapChipIndex].ViewChip()) {
              packChipBuckets[packView.currChipIndex].AddChip();
              folderChipSlots[folderView.swapChipIndex].GetChip(copy);

              gotChip = true;
            }
            else if (packChipBuckets[packView.currChipIndex].GetChip(copy)) {
              Chip prev;

              bool findBucket = folderChipSlots[folderView.swapChipIndex].GetChip(prev);

              folderChipSlots[folderView.swapChipIndex].AddChip(copy);

              // If the chip slot had a chip, find the corresponding bucket to add it back into
              if (findBucket) {
                auto iter = std::find_if(packChipBuckets.begin(), packChipBuckets.end(),
                  [&prev](const PackBucket& in) { return prev.GetShortName() == in.ViewChip().GetShortName(); }
                );

                if (iter != packChipBuckets.end()) {
                  iter->AddChip();
                }
              }

              gotChip = true;
            }

            if (gotChip) {
              packView.swapChipIndex = -1;
              folderView.swapChipIndex = -1;

              hasFolderChanged = true;

              AUDIO.Play(AudioType::CHIP_CONFIRM);
            }
            else {
              AUDIO.Play(AudioType::CHIP_ERROR);
            }
          }
          else {
            // select the chip
            packView.swapChipIndex = packView.currChipIndex;
            AUDIO.Play(AudioType::CHIP_CHOOSE);
          }
        }
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT) && currViewMode == ViewMode::FOLDER) {
      currViewMode = ViewMode::PACK;
      canInteract = false;
      AUDIO.Play(AudioType::CHIP_DESC);
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_LEFT) && currViewMode == ViewMode::PACK) {
      currViewMode = ViewMode::FOLDER;
      canInteract = false;
      AUDIO.Play(AudioType::CHIP_DESC);
    }

    view->currChipIndex = std::max(0, view->currChipIndex);
    view->currChipIndex = std::min(view->numOfChips - 1, view->currChipIndex);

    view->lastChipOnScreen = std::max(0, view->lastChipOnScreen);
    view->lastChipOnScreen = std::min(view->numOfChips - 1, view->lastChipOnScreen);

    bool gotoLastScene = false;

    if (INPUT.Has(EventTypes::PRESSED_CANCEL) && canInteract) {
      if (packView.swapChipIndex != -1 || folderView.swapChipIndex != -1) {
        AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
        packView.swapChipIndex = folderView.swapChipIndex = -1;
      }
      else  {
        WriteNewFolderData();

        gotoLastScene = true;
        canInteract = false;        
        
        AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      }
    }

    if (gotoLastScene) {
      canInteract = false;
      using swoosh::intent::direction;
      using segue = swoosh::intent::segue<BlackWashFade, swoosh::intent::milli<500>>;
      getController().queuePop<segue>();
    }
  } // end if(gotoLastScene)
  else {
    if (prevViewMode != currViewMode) {
      if (currViewMode == ViewMode::FOLDER) {
        if (camera.GetView().getCenter().x > 240) {
          camera.OffsetCamera(sf::Vector2f(-960.0f * 2.0f * float(elapsed), 0));
        }
        else {
          prevViewMode = currViewMode;
          canInteract = true;
        }
      }
      else if (currViewMode == ViewMode::PACK) {
        if (camera.GetView().getCenter().x < 720) {
          camera.OffsetCamera(sf::Vector2f(960.0f * 2.0f * float(elapsed), 0));
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

  float scale = 0.0f;

  ENGINE.Draw(folderChipCountBox);

  if(int(0.5+folderChipCountBox.getScale().y) == 2) {
    auto nonempty = (decltype(folderChipSlots))(folderChipSlots.size());
    auto iter = std::copy_if(folderChipSlots.begin(), folderChipSlots.end(), nonempty.begin(), [](auto in) { return !in.IsEmpty(); });
    nonempty.resize(std::distance(nonempty.begin(), iter));  // shrink container to new size

    std::string str = std::to_string(nonempty.size());
    // Draw number of chips in this folder
    chipLabel->setString(str);
    chipLabel->setOrigin(chipLabel->getLocalBounds().width, 0);
    chipLabel->setPosition(410.f, 1.f);

    if (nonempty.size() == 30) {
      chipLabel->setFillColor(sf::Color::Green);
    }
    else {
      chipLabel->setFillColor(sf::Color::White);
    }

    ENGINE.Draw(chipLabel, false);

    // Draw max
    chipLabel->setString(std::string("/ 30"));
    chipLabel->setOrigin(0, 0);;
    chipLabel->setPosition(415.f, 1.f);

    ENGINE.Draw(chipLabel, false);

    // reset, we use this label everywhere in this scene...
    chipLabel->setFillColor(sf::Color::White);

  }

  // folder chip count opens on FOLDER view mode only
  if (currViewMode == ViewMode::FOLDER) {
    if (prevViewMode == currViewMode) { // camera pan finished
      scale = swoosh::ease::interpolate((float)frameElapsed*8.f, 2.0f, folderChipCountBox.getScale().y);
    }
  }
  else {
    scale = swoosh::ease::interpolate((float)frameElapsed*8.f, 0.0f, folderChipCountBox.getScale().y);
  }

  folderChipCountBox.setScale(2.0f, scale);

  DrawFolder();
  DrawLibrary();
}

void FolderEditScene::DrawFolder() {
  chipDesc->setPosition(sf::Vector2f(20.f, 185.0f));
  scrollbar.setPosition(410.f, 60.f);
  chipHolder.setPosition(4.f, 35.f);
  element.setPosition(2.f*25.f, 146.f);
  chip.setPosition(83.f, 93.f);

  ENGINE.Draw(folderDock);
  ENGINE.Draw(chipHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)folderView.lastChipOnScreen / (float)folderView.numOfChips)*bottom;
  scrollbar.setPosition(452.f, top + depth);

  ENGINE.Draw(scrollbar);

  if (folder.GetSize() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  auto iter = folderChipSlots.begin();

  for (int j = 0; j < folderView.lastChipOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < folderView.maxChipsOnScreen && folderView.lastChipOnScreen + i < folderView.numOfChips; i++) {
    const Chip& copy = iter->ViewChip();

    if (!iter->IsEmpty()) {
      chipIcon.setTextureRect(TEXTURES.GetIconRectFromID(copy.GetIconID()));
      chipIcon.setPosition(2.f*104.f, 65.0f + (32.f*i));
      ENGINE.Draw(chipIcon, false);

      chipLabel->setFillColor(sf::Color::White);
      chipLabel->setPosition(2.f*120.f, 60.0f + (32.f*i));
      chipLabel->setString(copy.GetShortName());
      ENGINE.Draw(chipLabel, false);

      int offset = (int)(copy.GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*183.f, 65.0f + (32.f*i));
      ENGINE.Draw(element, false);

      chipLabel->setOrigin(0, 0);
      chipLabel->setPosition(2.f*200.f, 60.0f + (32.f*i));
      chipLabel->setString(std::string() + copy.GetCode());
      ENGINE.Draw(chipLabel, false);

      //Draw MB
      mbPlaceholder.setPosition(2.f*210.f, 67.0f + (32.f*i));
      ENGINE.Draw(mbPlaceholder, false);
    }

    // Draw cursor
    if (folderView.lastChipOnScreen + i == folderView.currChipIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, folderCursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*5.0f;

      folderCursor.setPosition((2.f*90.f) + bounce, y);
      ENGINE.Draw(folderCursor);

      if (!iter->IsEmpty()) {

        sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(copy.GetID());
        chip.setTextureRect(cardSubFrame);
        chip.setScale((float)swoosh::ease::linear(chipRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
        ENGINE.Draw(chip, false);

        // This draws the currently highlighted chip
        if (copy.GetDamage() > 0) {
          chipLabel->setFillColor(sf::Color::White);
          chipLabel->setString(std::to_string(copy.GetDamage()));
          chipLabel->setOrigin(chipLabel->getLocalBounds().width + chipLabel->getLocalBounds().left, 0);
          chipLabel->setPosition(2.f*(70.f), 135.f);

          ENGINE.Draw(chipLabel, false);
        }

        chipLabel->setOrigin(0, 0);
        chipLabel->setFillColor(sf::Color::Yellow);
        chipLabel->setPosition(2.f*14.f, 135.f);
        chipLabel->setString(std::string() + copy.GetCode());
        ENGINE.Draw(chipLabel, false);

        std::string formatted = FormatChipDesc(copy.GetDescription());
        chipDesc->setString(formatted);
        ENGINE.Draw(chipDesc, false);

        int offset = (int)(copy.GetElement());
        element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
        element.setPosition(2.f*25.f, 142.f);
        ENGINE.Draw(element, false);
      }
    }
    
    if (folderView.lastChipOnScreen + i == folderView.swapChipIndex && (int(totalTimeElapsed*1000) % 2 == 0)) {
      auto y =  64.0f + (32.f*i);

      folderSwapCursor.setPosition((2.f*95.f) + 2.0f, y);
      folderSwapCursor.setColor(sf::Color(255, 255, 255, 200));
      ENGINE.Draw(folderSwapCursor);
      folderSwapCursor.setColor(sf::Color::White);
    }

    iter++;
  }
}

void FolderEditScene::DrawLibrary() {
  chipDesc->setPosition(sf::Vector2f(326.f + 480.f, 185.0f));
  chipHolder.setPosition(310.f + 480.f, 35.f);
  element.setPosition(400.f + 2.f*20.f + 480.f, 146.f);
  chip.setPosition(389.f + 480.f, 93.f);

  ENGINE.Draw(packDock);
  ENGINE.Draw(chipHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)packView.lastChipOnScreen / (float)packView.numOfChips)*bottom;
  scrollbar.setPosition(292.f + 480.f, top + depth);

  ENGINE.Draw(scrollbar);

  if (CHIPLIB.GetSize() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  auto iter = packChipBuckets.begin();

  for (int j = 0; j < packView.lastChipOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < packView.maxChipsOnScreen && packView.lastChipOnScreen + i < packView.numOfChips; i++) {
    int count = iter->GetCount();
    const Chip& copy = iter->ViewChip();

    chipIcon.setTextureRect(TEXTURES.GetIconRectFromID(copy.GetIconID()));
    chipIcon.setPosition(19.f + 480.f, 65.0f + (32.f*i));
    ENGINE.Draw(chipIcon, false);

    chipLabel->setFillColor(sf::Color::White);
    chipLabel->setPosition(50.f + 480.f, 60.0f + (32.f*i));
    chipLabel->setString(copy.GetShortName());
    ENGINE.Draw(chipLabel, false);


    int offset = (int)(copy.GetElement());
    element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
    element.setPosition(163.0f + 480.f, 65.0f + (32.f*i));
    ENGINE.Draw(element, false);

    chipLabel->setOrigin(0, 0);
    chipLabel->setPosition(196.f + 480.f, 60.0f + (32.f*i));
    chipLabel->setString(std::string() + copy.GetCode());
    ENGINE.Draw(chipLabel, false);

    // Draw count in pack
    chipLabel->setOrigin(0, 0);
    chipLabel->setPosition(275.f + 480.f, 60.0f + (32.f*i));
    chipLabel->setString(std::to_string(count));
    ENGINE.Draw(chipLabel, false);

    //Draw MB
    mbPlaceholder.setPosition(220.f + 480.f, 67.0f + (32.f*i));
    ENGINE.Draw(mbPlaceholder, false);

    // Draw cursor
    if (packView.lastChipOnScreen + i == packView.currChipIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, packCursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*2.0f;

      packCursor.setPosition(bounce + 480.f + 2.f, y);
      ENGINE.Draw(packCursor);

      sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(copy.GetID());
      chip.setTextureRect(cardSubFrame);
      chip.setScale((float)swoosh::ease::linear(chipRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
      ENGINE.Draw(chip, false);

      // This draws the currently highlighted chip
      if (copy.GetDamage() > 0) {
        chipLabel->setFillColor(sf::Color::White);
        chipLabel->setString(std::to_string(copy.GetDamage()));
        chipLabel->setOrigin(chipLabel->getLocalBounds().width + chipLabel->getLocalBounds().left, 0);
        chipLabel->setPosition(2.f*(223.f) + 480.f, 135.f);

        ENGINE.Draw(chipLabel, false);
      }

      chipLabel->setOrigin(0, 0);
      chipLabel->setFillColor(sf::Color::Yellow);
      chipLabel->setPosition(2.f*167.f + 480.f, 135.f);
      chipLabel->setString(std::string() + copy.GetCode());
      ENGINE.Draw(chipLabel, false);

      std::string formatted = FormatChipDesc(copy.GetDescription());
      chipDesc->setString(formatted);
      ENGINE.Draw(chipDesc, false);

      int offset = (int)(copy.GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*179.f + 480.f, 142.f);
      ENGINE.Draw(element, false);
    }
    
    if (packView.lastChipOnScreen + i == packView.swapChipIndex && (int(totalTimeElapsed*1000) % 2 == 0)) {
      auto y =  64.0f + (32.f*i);

      packSwapCursor.setPosition(485.f + 2.f + 2.f, y);
      packSwapCursor.setColor(sf::Color(255, 255, 255, 200));
      ENGINE.Draw(packSwapCursor);
      packSwapCursor.setColor(sf::Color::White);
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

void FolderEditScene::ExcludeFolderDataFromPack()
{
  Chip mock; // will not be used
  for (auto& f : folderChipSlots) {
    auto iter = std::find_if(packChipBuckets.begin(), packChipBuckets.end(), [&f](PackBucket& pack) { return pack.ViewChip() == f.ViewChip(); });
    if (iter != packChipBuckets.end()) {
      iter->GetChip(mock);
    }
  }
}

void FolderEditScene::PlaceFolderDataIntoChipSlots()
{
  ChipFolder::Iter iter = folder.Begin();
  
  while (iter != folder.End() && folderChipSlots.size() < 30) {
    auto slot = FolderSlot();
    slot.AddChip(Chip(**iter));
    folderChipSlots.push_back(slot);
    iter++;
  }

  while (folderChipSlots.size() < 30) {
    folderChipSlots.push_back({});
  }
}

void FolderEditScene::PlaceLibraryDataIntoBuckets()
{
  ChipLibrary::Iter iter = CHIPLIB.Begin();
  std::map<Chip, bool, Chip::Compare> visited; // visit table

  while (iter != CHIPLIB.End()) {
    if (visited.find(*iter) != visited.end()) {
      iter++;
      continue;
    }

    visited[(*iter)] = true;
    int count = CHIPLIB.GetCountOf(*iter);
    auto bucket = PackBucket(count, Chip(*iter));
    packChipBuckets.push_back(bucket);
    iter++;
  }
}

void FolderEditScene::WriteNewFolderData()
{
  folder = ChipFolder();

  for (auto iter = folderChipSlots.begin(); iter != folderChipSlots.end(); iter++) {
    if ((*iter).IsEmpty()) continue; 

    folder.AddChip((*iter).ViewChip());
  }
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
