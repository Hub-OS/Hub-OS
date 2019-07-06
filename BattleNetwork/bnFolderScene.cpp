#include <time.h>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderScene.h"
#include "bnChipLibrary.h"
#include "bnChipFolder.h"
#include "Android/bnTouchArea.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#include "Segues/PushIn.h"

FolderScene::FolderScene(swoosh::ActivityController &controller, ChipFolderCollection& collection) :
  collection(collection),
  camera(ENGINE.GetDefaultView()),
  swoosh::Activity(&controller)
{
  promptOptions = false;
  enterText = false;
  gotoNextScene = true;

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("Folders", *font);
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

  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_INFO_BG));
  bg.setScale(2.f, 2.f);

  scrollbar = sf::Sprite(LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);
  scrollbar.setPosition(410.f, 60.f);

  folderBox = sf::Sprite(LOAD_TEXTURE(FOLDER_BOX));
  folderBox.setScale(2.f, 2.f);

  folderOptions = sf::Sprite(LOAD_TEXTURE(FOLDER_OPTIONS));
  folderOptions.setOrigin(folderOptions.getGlobalBounds().width / 2.0f, folderOptions.getGlobalBounds().height / 2.0f);
  folderOptions.setPosition(98.0f, 210.0f);
  folderOptions.setScale(2.f, 0.f); // hide on start

  folderCursor = sf::Sprite(LOAD_TEXTURE(FOLDER_BOX_CURSOR));
  folderCursor.setScale(2.f, 2.f);

  folderEquip = sf::Sprite(LOAD_TEXTURE(FOLDER_EQUIP));
  folderEquip.setScale(2.f, 2.f);

  cursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_CURSOR));
  cursor.setScale(2.f, 2.f);
  cursor.setPosition(2.0, 155.0f);

  element = sf::Sprite(LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  chipIcon = sf::Sprite(LOAD_TEXTURE(CHIP_ICONS));
  chipIcon.setScale(2.f, 2.f);

  equipAnimation = Animation("resources/ui/folder_equip.animation");
  equipAnimation.SetAnimation("BLINK");
  equipAnimation << Animate::Mode::Loop;

  folderCursorAnimation = Animation("resources/ui/folder_cursor.animation");
  folderCursorAnimation.SetAnimation("BLINK");
  folderCursorAnimation << Animate::Mode::Loop;

  equipAnimation.Update(0,folderEquip);
  folderCursorAnimation.Update(0, folderCursor);


  maxChipsOnScreen = 5;
  currChipIndex = 0;
  currFolderIndex = lastFolderIndex = 0;
  selectedFolderIndex = optionIndex = 0;

  totalTimeElapsed = frameElapsed = folderOffsetX = 0.0;

  folder = nullptr;

  folderNames = collection.GetFolderNames();

  if (collection.GetFolderNames().size() > 0) {
    collection.GetFolder(*folderNames.begin(), folder);

    numOfChips = folder->GetSize();
  }
  else {
    numOfChips = 0;
  }

#ifdef __ANDROID__
  canSwipe = true;
  touchStart = false;
  touchPosX = touchPosStartX = 0;
  releasedB = false;
#endif
}

FolderScene::~FolderScene() { ; }

void FolderScene::onStart() {
  ENGINE.SetCamera(camera);

  gotoNextScene = false;

#ifdef __ANDROID__
    this->StartupTouchControls();
#endif
}

void FolderScene::onUpdate(double elapsed) {
  bool folderSwitch = false;

  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  camera.Update((float)elapsed);
  folderCursorAnimation.Update((float)elapsed, folderCursor);
  equipAnimation.Update((float)elapsed, folderEquip);

  // Scene keyboard controls
  if (enterText) {
    InputEvent  cancelButton = RELEASED_B;

#ifdef __ANDROID__
    cancelButton = PRESSED_B;
#endif

    if (INPUT.Has(cancelButton)) {
#ifdef __ANDROID__
        sf::Keyboard::setVirtualKeyboardVisible(false);
#endif
      this->enterText = false;
      bool changed = collection.SetFolderName(folderNames[currFolderIndex], folder);

      if (!changed) {
        folderNames[currFolderIndex] = collection.GetFolderNames()[currFolderIndex];
      }

      INPUT.EndCaptureInputBuffer();
    }
    else {
      std::string buffer = INPUT.GetInputBuffer();

      buffer = buffer.substr(0, 10);
      folderNames[currFolderIndex] = buffer;

      INPUT.SetInputBuffer(buffer); // shrink

#ifdef __ANDROID__
        sf::Keyboard::setVirtualKeyboardVisible(true);
#endif
    }
  } else if (!gotoNextScene) {
      if (INPUT.Has(PRESSED_UP)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currChipIndex--;
          }
          else {
            optionIndex--;
          }

          AUDIO.Play(AudioType::CHIP_SELECT);
        }
      }
      else if (INPUT.Has(PRESSED_DOWN)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currChipIndex++;
          }
          else {
            optionIndex++;
          }

          AUDIO.Play(AudioType::CHIP_SELECT);
        }
      }
      else if (INPUT.Has(PRESSED_RIGHT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currFolderIndex++;
            folderSwitch = true;
            AUDIO.Play(AudioType::CHIP_SELECT);
          }
        }
      }
      else if (INPUT.Has(PRESSED_LEFT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currFolderIndex--;
            folderSwitch = true;
            AUDIO.Play(AudioType::CHIP_SELECT);
          }
        }
      }
      else {
        selectInputCooldown = 0;
      }

      currChipIndex = std::max(0, currChipIndex);
      currChipIndex = std::min(numOfChips - 1, currChipIndex);
      currFolderIndex = std::max(0, currFolderIndex);
      currFolderIndex = std::min((int)folderNames.size() - 1, currFolderIndex);
      optionIndex = std::max(0, optionIndex);
      optionIndex = std::min(2, optionIndex);

#ifdef __ANDROID__
      if(lastFolderIndex != currFolderIndex) {
          folderSwitch = true;
      }
#endif

      if (folderSwitch) {
        if (collection.GetFolderNames().size() > 0) {
          collection.GetFolder(*(folderNames.begin() + currFolderIndex), folder);

          numOfChips = folder->GetSize();
          currChipIndex = 0;
        }
      }

      InputEvent  cancelButton = RELEASED_B;

      if (INPUT.Has(PRESSED_B)) {
        if (!promptOptions) {
          gotoNextScene = true;
          AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

          using swoosh::intent::direction;
          using segue = swoosh::intent::segue<PushIn<direction::right>>;
          getController().queuePop<segue>();
        } else {
            promptOptions = false;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
        }
      } else if (INPUT.Has(RELEASED_B)) {
          if (promptOptions) {
            promptOptions = false;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
          }
      } else if (INPUT.Has(PRESSED_A)) {
        if (!promptOptions) {
          promptOptions = true;
          AUDIO.Play(AudioType::CHIP_DESC);
        }
        else {
          switch (optionIndex) {
          case 0: // EDIT
            this->enterText = true;
            INPUT.BeginCaptureInputBuffer();
            INPUT.SetInputBuffer(*(folderNames.begin() + currFolderIndex));
            break;
          case 1: // EQUIP
            selectedFolderIndex = currFolderIndex;
            AUDIO.Play(AudioType::PA_ADVANCE);
            break;
          case 2: // CHANGE NAME
            break;
          }
        }
      }
  }

#ifdef __ANDROID__
  if(canSwipe) {
      Logger::Log("touch is down");

      if (sf::Touch::isDown(0)) {

      sf::Vector2i touchPosition = sf::Touch::getPosition(0, *ENGINE.GetWindow());
      sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition,
                                                                 ENGINE.GetDefaultView());
      sf::Vector2i iCoords = sf::Vector2i((int) coords.x, (int) coords.y);
      touchPosition = iCoords;

      canSwipe = false;

      if(touchPosition.y < 100) {
          if (!touchStart) {
              touchStart = true;
              touchPosStartX = touchPosition.x;
          }

          touchPosX = touchPosition.x;
          folderOffsetX = (touchPosStartX - touchPosX);
          Logger::Log("folderOffsetX: " + std::to_string(folderOffsetX));

          canSwipe = true;
      } else if(folderOffsetX > 100){
          currFolderIndex++;
          canSwipe = true;
      } else if(folderOffsetX < -100) {
          currFolderIndex--;
          canSwipe = true;
      }
    } else {
      canSwipe = true;
      touchStart = false;
    }
  }
#endif
}

void FolderScene::onLeave() {
#ifdef __ANDROID__
    this->ShutdownTouchControls();
#endif
}

void FolderScene::onExit()
{
}

void FolderScene::onEnter()
{
}

void FolderScene::onResume() {
#ifdef __ANDROID__
    this->StartupTouchControls();
#endif
}

void FolderScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  if (folderNames.size() > 0) {
    for (int i = 0; i < folderNames.size(); i++) {
      folderBox.setPosition(26.0f + (i*144.0f) - (float)folderOffsetX, 34.0f);
      ENGINE.Draw(folderBox);

      chipLabel->setFillColor(sf::Color::White);
      chipLabel->setString(folderNames[i]);
      chipLabel->setOrigin(chipLabel->getGlobalBounds().width / 2.0f, chipLabel->getGlobalBounds().height / 2.0f);
      chipLabel->setPosition(95.0f + (i*144.0f) - (float)folderOffsetX, 50.0f);
      ENGINE.Draw(chipLabel, false);

      if (i == selectedFolderIndex) {
        folderEquip.setPosition(25.0f + (i*144.0f) - (float)folderOffsetX, 30.0f);
        ENGINE.Draw(folderEquip, false);
      }

    }

    if(!canSwipe) {
      auto x = swoosh::ease::interpolate((float) frameElapsed * 7.f, folderCursor.getPosition().x,
                                         98.0f + (std::min(2, currFolderIndex) * 144.0f));
      folderCursor.setPosition(x, 68.0f);

      if (currFolderIndex > 2) {
          auto before = folderOffsetX;
        folderOffsetX = swoosh::ease::interpolate(frameElapsed * 7.0, folderOffsetX,
                                                  (double) (((currFolderIndex - 2) * 144.0f)));

        if(int(before) == int(folderOffsetX)) {
            canSwipe = true;
            touchStart = false;
        }
      } else {
          auto before = folderOffsetX;
        folderOffsetX = swoosh::ease::interpolate(frameElapsed * 7.0, folderOffsetX, 0.0);

          if(int(before) == int(folderOffsetX)) {
              canSwipe = true;
              touchStart = false;
          }
      }
    }

    ENGINE.Draw(folderCursor, false);
  }

  // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
  float top = 120.0f; float bottom = 170.0f;
  float depth = ((float)(currChipIndex) / (float)numOfChips)*bottom;
  scrollbar.setPosition(436.f, top + depth);

  ENGINE.Draw(scrollbar);

  if (!folder) return;
  if (folder && folder->GetSize() == 0) return;

  // Move the chip library iterator to the current highlighted chip
  ChipFolder::Iter iter = folder->Begin();

  for (int j = 0; j < currChipIndex; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each chip in the list
  for (int i = 0; i < maxChipsOnScreen && currChipIndex + i < numOfChips; i++) {
    chipIcon.setTextureRect(TEXTURES.GetIconRectFromID(((*iter)->GetIconID())));
    chipIcon.setPosition(2.f*104.f, 133.0f + (32.f*i));
    ENGINE.Draw(chipIcon, false);

    chipLabel->setOrigin(0.0f, 0.0f);
    chipLabel->setFillColor(sf::Color::White);
    chipLabel->setPosition(2.f*120.f, 128.0f + (32.f*i));
    chipLabel->setString((*iter)->GetShortName());
    ENGINE.Draw(chipLabel, false);


    int offset = (int)((*iter)->GetElement());
    element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
    element.setPosition(2.f*173.f, 133.0f + (32.f*i));
    ENGINE.Draw(element, false);

    chipLabel->setOrigin(0, 0);
    chipLabel->setPosition(2.f*190.f, 128.0f + (32.f*i));
    chipLabel->setString(std::string() + (*iter)->GetCode());
    ENGINE.Draw(chipLabel, false);

    iter++;
  }

  auto y = swoosh::ease::interpolate((float)frameElapsed*7.0f, cursor.getPosition().y, 170.0f + ((optionIndex)*32.0f));
  cursor.setPosition(2.0, y);

  float scale = 0.0f;

  ENGINE.Draw(folderOptions);


  if (promptOptions) {
   scale = swoosh::ease::interpolate((float)frameElapsed*4.0f, 2.0f, folderOptions.getScale().y);
   ENGINE.Draw(cursor);
  }
  else {
    scale = swoosh::ease::interpolate((float)frameElapsed*4.0f, 0.0f, folderOptions.getScale().y);
  }

  folderOptions.setScale(2.0f, scale);
}

void FolderScene::onEnd() {
  delete font;
  delete chipFont;
  delete numberFont;
  delete menuLabel;
  delete numberLabel;

#ifdef __ANDROID__
    this->ShutdownTouchControls();
#endif
}

#ifdef __ANDROID__
void FolderScene::StartupTouchControls() {
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

        this->releasedB = false;
    });

    rightSide.onDrag([this](sf::Vector2i delta){
        if(delta.x < -25 && !this->releasedB && !touchStart) {
            INPUT.VirtualKeyEvent(InputEvent::PRESSED_B);
            INPUT.VirtualKeyEvent(InputEvent::RELEASED_B);
            this->releasedB = true;
        }
    });
}

void FolderScene::ShutdownTouchControls() {
    TouchArea::free();
}
#endif