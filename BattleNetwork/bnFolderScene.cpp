#include <time.h>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderScene.h"
#include "bnFolderEditScene.h"
#include "bnFolderChangeNameScene.h"
#include "Segues/BlackWashFade.h"
#include "bnCardLibrary.h"
#include "bnCardFolder.h"
#include "Android/bnTouchArea.h"
#include "bnMessageQuestion.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

using namespace swoosh::types;

#include "Segues/PushIn.h"

FolderScene::FolderScene(swoosh::ActivityController &controller, CardFolderCollection& collection) :
  collection(collection),
  camera(ENGINE.GetView()),
  folderSwitch(true),
  textbox(sf::Vector2f(4, 255)),
  swoosh::Activity(&controller)
{
  textbox.SetTextSpeed(2.0f);

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

  // Card UI font
  cardFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  cardLabel = new sf::Text("", *cardFont);
  cardLabel->setPosition(275.f, 15.f);

  numberFont = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  numberLabel = new sf::Text("", *numberFont);
  numberLabel->setOutlineColor(sf::Color(48, 56, 80));
  numberLabel->setOutlineThickness(2.f);
  numberLabel->setScale(0.8f, 0.8f);
  numberLabel->setOrigin(numberLabel->getLocalBounds().width, 0);
  numberLabel->setPosition(sf::Vector2f(170.f, 28.0f));

  // folder menu graphic
  bg = sf::Sprite(*LOAD_TEXTURE(FOLDER_INFO_BG));
  bg.setScale(2.f, 2.f);

  scrollbar = sf::Sprite(*LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);
  scrollbar.setPosition(410.f, 60.f);

  folderBox = sf::Sprite(*LOAD_TEXTURE(FOLDER_BOX));
  folderBox.setScale(2.f, 2.f);

  folderOptions = sf::Sprite(*LOAD_TEXTURE(FOLDER_OPTIONS));
  folderOptions.setOrigin(folderOptions.getGlobalBounds().width / 2.0f, folderOptions.getGlobalBounds().height / 2.0f);
  folderOptions.setPosition(98.0f, 210.0f);
  folderOptions.setScale(2.f, 0.f); // hide on start

  folderCursor = sf::Sprite(*LOAD_TEXTURE(FOLDER_BOX_CURSOR));
  folderCursor.setScale(2.f, 2.f);

  folderEquip = sf::Sprite(*LOAD_TEXTURE(FOLDER_EQUIP));
  folderEquip.setScale(2.f, 2.f);

  cursor = sf::Sprite(*LOAD_TEXTURE(TEXT_BOX_CURSOR));
  cursor.setScale(2.f, 2.f);
  cursor.setPosition(2.0, 155.0f);

  element = sf::Sprite(*LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  cardIcon = sf::Sprite();
  cardIcon.setScale(2.f, 2.f);
  cardIcon.setTextureRect(sf::IntRect(0, 0, 14, 14));

  mbPlaceholder = sf::Sprite(*LOAD_TEXTURE(FOLDER_MB));
  mbPlaceholder.setScale(2.f, 2.f);

  equipAnimation = Animation("resources/ui/folder_equip.animation");
  equipAnimation.SetAnimation("BLINK");
  equipAnimation << Animator::Mode::Loop;

  folderCursorAnimation = Animation("resources/ui/folder_cursor.animation");
  folderCursorAnimation.SetAnimation("BLINK");
  folderCursorAnimation << Animator::Mode::Loop;

  equipAnimation.Update(0,folderEquip);
  folderCursorAnimation.Update(0, folderCursor);

  maxCardsOnScreen = 5;
  currCardIndex = 0;
  currFolderIndex = lastFolderIndex = 0;
  selectedFolderIndex = optionIndex = 0;

  totalTimeElapsed = frameElapsed = folderOffsetX = 0.0;

  folder = nullptr;

  folderNames = collection.GetFolderNames();

  if (collection.GetFolderNames().size() > 0) {
    collection.GetFolder(0, folder);

    numOfCards = folder->GetSize();
  }
  else {
    numOfCards = 0;
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
    StartupTouchControls();
#endif
}

void FolderScene::onUpdate(double elapsed) {
  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  camera.Update((float)elapsed);
  folderCursorAnimation.Update((float)elapsed, folderCursor);
  equipAnimation.Update((float)elapsed, folderEquip);
  textbox.Update(elapsed);

  auto lastCardIndex = currCardIndex;
  auto lastFolderIndex = currFolderIndex;
  auto lastOptionIndex = optionIndex;

  // Prioritize textbox input
  if (textbox.IsOpen()) return;

  // Scene keyboard controls
  if (enterText) {
    InputEvent  cancelButton = EventTypes::RELEASED_CANCEL;

#ifdef __ANDROID__
    cancelButton = PRESSED_B;
#endif

    if (INPUT.Has(cancelButton)) {
#ifdef __ANDROID__
        sf::Keyboard::setVirtualKeyboardVisible(false);
#endif
      enterText = false;
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
      if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currCardIndex--;
          }
          else {
            optionIndex--;
          }
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currCardIndex++;
          }
          else {
            optionIndex++;
          }
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currFolderIndex++;
            folderSwitch = true;
          }
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          selectInputCooldown = maxSelectInputCooldown;

          if (!promptOptions) {
            currFolderIndex--;
            folderSwitch = true;
          }
        }
      }
      else {
        selectInputCooldown = 0;
      }

      currCardIndex = std::max(0, currCardIndex);
      currCardIndex = std::min(numOfCards - 1, currCardIndex);
      currFolderIndex = std::max(0, currFolderIndex);
      currFolderIndex = std::min((int)folderNames.size() - 1, currFolderIndex);
      optionIndex = std::max(0, optionIndex);

      if (currCardIndex != lastCardIndex 
        || currFolderIndex != lastFolderIndex 
        || optionIndex != lastOptionIndex) {
        AUDIO.Play(AudioType::CHIP_SELECT);
      }

      if (folderNames.size()) {
        optionIndex = std::min(4, optionIndex); // total of 5 options 
      }
      else {
        optionIndex = 0; // "NEW" option only
      }

#ifdef __ANDROID__
      if(lastFolderIndex != currFolderIndex) {
          folderSwitch = true;
      }
#endif

      if (folderSwitch) {
        if (collection.GetFolderNames().size() > 0) {
          collection.GetFolder(*(folderNames.begin() + currFolderIndex), folder);

          numOfCards = folder->GetSize();
          currCardIndex = 0;
        }

        folderSwitch = false;
      }

      InputEvent cancelButton = EventTypes::RELEASED_CANCEL;

      if (INPUT.Has(EventTypes::PRESSED_CANCEL)) {
        if (!promptOptions) {
          gotoNextScene = true;
          AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

          using segue = segue<PushIn<direction::right>, milli<500>>;
          getController().queuePop<segue>();
        } else {
            promptOptions = false;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
        }
      } else if (INPUT.Has(EventTypes::PRESSED_CANCEL)) {
          if (promptOptions) {
            promptOptions = false;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
          }
      } else if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
        if (!promptOptions) {
          promptOptions = true;
          AUDIO.Play(AudioType::CHIP_DESC);
        }
        else if(folderNames.size()) {
          switch (optionIndex) {
          case 0: // EDIT
            if (folder) {
              using effect = segue<BlackWashFade, milli<500>>;
              getController().push<effect::to<FolderEditScene>>(*folder);

              AUDIO.Play(AudioType::CHIP_CONFIRM);
              gotoNextScene = true;
            }
            else {
              AUDIO.Play(AudioType::CHIP_ERROR);
            }
            break;
          case 1: // EQUIP
            selectedFolderIndex = currFolderIndex;
            collection.SwapOrder(0, selectedFolderIndex);
            AUDIO.Play(AudioType::PA_ADVANCE);
            break;
          case 2: // CHANGE NAME
            if (folder) {
              using effect = segue<BlackWashFade, milli<500>>;
              getController().push<effect::to<FolderChangeNameScene>>(folderNames[currFolderIndex]);
              AUDIO.Play(AudioType::CHIP_CONFIRM);
              gotoNextScene = true;
            }
            else {
              AUDIO.Play(AudioType::CHIP_ERROR);
            }
            break;
          case 3: // NEW 
            MakeNewFolder();
            promptOptions = false;
            currFolderIndex = (int)folderNames.size() - 1;
            folderSwitch = true;

            break;
          case 4: // DELETE 
            DeleteFolder([this]() {
              promptOptions = false;
              currFolderIndex--;
              folderSwitch = true;
            });
            break;
          }
        }
        else {
          MakeNewFolder();
          promptOptions = false;
          currFolderIndex = (int)folderNames.size() - 1;
          folderSwitch = true;
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
    ShutdownTouchControls();
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
    StartupTouchControls();
#endif

    gotoNextScene = false;

    // Save any edits
    collection.SetFolderName(folderNames[currFolderIndex], folder);
    folderSwitch = true;
    collection.WriteToFile("resources/database/folders.txt");
}

void FolderScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  if (folderNames.size() > 0) {
    for (int i = 0; i < folderNames.size(); i++) {
      folderBox.setPosition(26.0f + (i*144.0f) - (float)folderOffsetX, 34.0f);
      ENGINE.Draw(folderBox);

      cardLabel->setFillColor(sf::Color::White);
      cardLabel->setString(folderNames[i]);
      cardLabel->setOrigin(cardLabel->getGlobalBounds().width / 2.0f, cardLabel->getGlobalBounds().height / 2.0f);
      cardLabel->setPosition(95.0f + (i*144.0f) - (float)folderOffsetX, 50.0f);
      ENGINE.Draw(cardLabel, false);

      if (i == selectedFolderIndex) {
        folderEquip.setPosition(25.0f + (i*144.0f) - (float)folderOffsetX, 30.0f);
        ENGINE.Draw(folderEquip, false);
      }

    }

#ifdef __ANDROID__
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
#else 
    auto x = swoosh::ease::interpolate((float)frameElapsed * 7.f, folderCursor.getPosition().x,
      98.0f + (std::min(2, currFolderIndex) * 144.0f));
    folderCursor.setPosition(x, 68.0f);

    if (currFolderIndex > 2) {
      auto before = folderOffsetX;
      folderOffsetX = swoosh::ease::interpolate(frameElapsed * 7.0, folderOffsetX,
        (double)(((currFolderIndex - 2) * 144.0f)));
    }
    else {
      auto before = folderOffsetX;
      folderOffsetX = swoosh::ease::interpolate(frameElapsed * 7.0, folderOffsetX, 0.0);
    }
#endif

    ENGINE.Draw(folderCursor, false);
  }

  // ScrollBar limits: Top to bottom screen position when selecting first and last card respectively
  float top = 120.0f; float bottom = 170.0f;
  float depth = ((float)(currCardIndex) / (float)numOfCards)*bottom;
  scrollbar.setPosition(436.f, top + depth);

  ENGINE.Draw(scrollbar);
  ENGINE.Draw(folderOptions);

  float scale = 0.0f;

  if (promptOptions) {
    scale = swoosh::ease::interpolate((float)frameElapsed*4.0f, 2.0f, folderOptions.getScale().y);
    ENGINE.Draw(cursor);
  }
  else {
    scale = swoosh::ease::interpolate((float)frameElapsed*4.0f, 0.0f, folderOptions.getScale().y);
  }

  folderOptions.setScale(2.0f, scale);


  auto y = swoosh::ease::interpolate((float)frameElapsed*7.0f, cursor.getPosition().y, 138.0f + ((optionIndex)*32.0f));
  cursor.setPosition(2.0, y);

  if (!folder) return;
  if (folder->GetSize() != 0) {

    // Move the card library iterator to the current highlighted card
    CardFolder::Iter iter = folder->Begin();

    for (int j = 0; j < currCardIndex; j++) {
      iter++;
    }

    cardLabel->setFillColor(sf::Color::White);
    cardLabel->setString(folderNames[currFolderIndex]);
    cardLabel->setOrigin(0.f, cardLabel->getGlobalBounds().height / 2.0f);
    cardLabel->setPosition(195.0f, 100.0f);
    ENGINE.Draw(cardLabel, false);

    // Now that we are at the viewing range, draw each card in the list
    for (int i = 0; i < maxCardsOnScreen && currCardIndex + i < numOfCards; i++) {
      cardIcon.setTexture(*WEBCLIENT.GetIconForCard((*iter)->GetUUID()));
      cardIcon.setPosition(2.f*99.f, 133.0f + (32.f*i));
      ENGINE.Draw(cardIcon, false);

      cardLabel->setOrigin(0.0f, 0.0f);
      cardLabel->setFillColor(sf::Color::White);
      cardLabel->setPosition(2.f*115.f, 128.0f + (32.f*i));
      cardLabel->setString((*iter)->GetShortName());
      ENGINE.Draw(cardLabel, false);


      int offset = (int)((*iter)->GetElement());
      element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
      element.setPosition(2.f*173.f, 133.0f + (32.f*i));
      ENGINE.Draw(element, false);

      cardLabel->setOrigin(0, 0);
      cardLabel->setPosition(2.f*190.f, 128.0f + (32.f*i));
      cardLabel->setString(std::string() + (*iter)->GetCode());
      ENGINE.Draw(cardLabel, false);

      mbPlaceholder.setPosition(2.f*200.f, 134.0f + (32.f*i));
      ENGINE.Draw(mbPlaceholder, false);
      iter++;
    }
  }

  ENGINE.Draw(textbox, false);
}

void FolderScene::MakeNewFolder() {
  AUDIO.Play(AudioType::CHIP_CONFIRM); 

  std::string name = "NewFldr";
  int i = 0;

  while (!collection.MakeFolder(name)) {
    i++;
    name = "NewFldr" + std::to_string(i);
  }

  folderNames = collection.GetFolderNames();
}

void FolderScene::DeleteFolder(std::function<void()> onSuccess)
{
  if (!folderNames.size()) {
    AUDIO.Play(AudioType::CHIP_ERROR);

    return;
  }

  auto onYes = [onSuccess, this]() {
    if (collection.DeleteFolder(folderNames[currFolderIndex])) {
      onSuccess();
      folderNames = collection.GetFolderNames();
    }

    textbox.Close();
    AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
  };

  auto onNo = [this]() {
    textbox.Close();
    AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
  };

  textbox.EnqueMessage(
    sf::Sprite(*LOAD_TEXTURE(MUG_NAVIGATOR)), 
    "resources/ui/navigator.animation", 
    new Question("Are you sure you want to permanently delete this folder?", 
    onYes,
    onNo));

  textbox.Open();
}

void FolderScene::onEnd() {
  delete menuLabel;
  delete numberLabel;

#ifdef __ANDROID__
    ShutdownTouchControls();
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
        if(!releasedB) {
            INPUT.VirtualKeyEvent(InputEvent::PRESSED_A);
        }

        releasedB = false;
    });

    rightSide.onDrag([this](sf::Vector2i delta){
        if(delta.x < -25 && !releasedB && !touchStart) {
            INPUT.VirtualKeyEvent(InputEvent::PRESSED_B);
            INPUT.VirtualKeyEvent(InputEvent::RELEASED_B);
            releasedB = true;
        }
    });
}

void FolderScene::ShutdownTouchControls() {
    TouchArea::free();
}
#endif
