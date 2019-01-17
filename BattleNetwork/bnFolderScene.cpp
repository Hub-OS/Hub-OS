#include <time.h>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderScene.h"
#include "bnChipLibrary.h"
#include "bnChipFolder.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#include "Segues\PushIn.h"

FolderScene::FolderScene(swoosh::ActivityController &controller, ChipFolderCollection& collection) :
  collection(collection),
  camera(ENGINE.GetDefaultView()),
  swoosh::Activity(controller)
{

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
  folderOptions.setScale(2.f, 2.f);

  folderCursor = sf::Sprite(LOAD_TEXTURE(FOLDER_BOX_CURSOR));
  folderCursor.setScale(2.f, 2.f);

  folderEquip = sf::Sprite(LOAD_TEXTURE(FOLDER_EQUIP));
  folderEquip.setScale(2.f, 2.f);

  cursor = sf::Sprite(LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);
  cursor.setPosition((2.f*90.f), 64.0f);

  element = sf::Sprite(LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  chipIcon = sf::Sprite(LOAD_TEXTURE(CHIP_ICONS));
  chipIcon.setScale(2.f, 2.f);

  equipAnimation = Animation("resources/ui/folder_equip.animation");
  equipAnimation.SetAnimation("BLINK");
  equipAnimation << Animate::Mode(Animate::Mode::Loop);

  folderCursorAnimation = Animation("resources/ui/folder_cursor.animation");
  folderCursorAnimation.SetAnimation("BLINK");
  folderCursorAnimation << Animate::Mode(Animate::Mode::Loop);

  maxChipsOnScreen = 5;
  currChipIndex = 0;
  currFolderIndex = 0;
  selectedFolderIndex = 0;

  totalTimeElapsed = frameElapsed = 0.0;

  folder = nullptr;

  folderNames = collection.GetFolderNames();

  if (collection.GetFolderNames().size() > 0) {
    collection.GetFolder(*folderNames.begin(), folder);

    numOfChips = folder->GetSize();
  }
  else {
    numOfChips = 0;
  }
}

FolderScene::~FolderScene() { ; }

void FolderScene::onStart() {
  ENGINE.SetCamera(camera);

  gotoNextScene = false;
}

void FolderScene::onUpdate(double elapsed) {
  bool folderSwitch = false;

  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  camera.Update((float)elapsed);
  folderCursorAnimation.Update((float)elapsed, &folderCursor);
  equipAnimation.Update((float)elapsed, &folderEquip);

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUT.has(PRESSED_UP)) {
      selectInputCooldown -= elapsed;


      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currChipIndex--;
        AUDIO.Play(AudioType::CHIP_SELECT);

      }
    }
    else if (INPUT.has(PRESSED_DOWN)) {
      selectInputCooldown -= elapsed;


      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currChipIndex++;
        AUDIO.Play(AudioType::CHIP_SELECT);
      }
    }
    else if (INPUT.has(PRESSED_RIGHT)) {
      selectInputCooldown -= elapsed;


      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currFolderIndex++;
        folderSwitch = true;
        AUDIO.Play(AudioType::CHIP_SELECT);
      }
    }
    else if (INPUT.has(PRESSED_LEFT)) {
      selectInputCooldown -= elapsed;


      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currFolderIndex--;
        folderSwitch = true;
        AUDIO.Play(AudioType::CHIP_SELECT);
      }
    }
    else {
      selectInputCooldown = 0;
    }

    currChipIndex = std::max(0, currChipIndex);
    currChipIndex = std::min(numOfChips - 1, currChipIndex);
    currFolderIndex = std::max(0, currFolderIndex);
    currFolderIndex = std::min((int)folderNames.size() - 1, currFolderIndex);

    if (folderSwitch) {
      if (collection.GetFolderNames().size() > 0) {
        collection.GetFolder(*(folderNames.begin()+currFolderIndex), folder);

        numOfChips = folder->GetSize();
        currChipIndex = 0;
      }
    }

    if (INPUT.has(PRESSED_B)) {
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

      using swoosh::intent::direction;
      using segue = swoosh::intent::segue<PushIn<direction::right>>;
      getController().queuePop<segue>();
    }
  }
}

void FolderScene::onLeave() {

}

void FolderScene::onExit()
{
}

void FolderScene::onEnter()
{
}

void FolderScene::onResume() {

}

void FolderScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  ENGINE.DrawUnderlay();
  ENGINE.DrawLayers();
  ENGINE.DrawOverlay();

  if (folderNames.size() > 0) {
    for (int i = 0; i < folderNames.size(); i++) {
      folderBox.setPosition(26.0f + (i*144.0f), 34.0f);
      ENGINE.Draw(folderBox);

      chipLabel->setFillColor(sf::Color::White);
      chipLabel->setString(folderNames[i]);
      chipLabel->setOrigin(chipLabel->getGlobalBounds().width / 2.0f, chipLabel->getGlobalBounds().height / 2.0f);
      chipLabel->setPosition(95.0f + (i*144.0f), 50.0f);
      ENGINE.Draw(chipLabel, false);

      if (i == selectedFolderIndex) {
        folderEquip.setPosition(25.0f + (i*144.0f), 30.0f);
        ENGINE.Draw(folderEquip, false);
      }

    }
    auto x = swoosh::ease::interpolate((float)frameElapsed*7.f, folderCursor.getPosition().x, 98.0f + (currFolderIndex*144.0f));
    folderCursor.setPosition(x, 68.0f);
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
}

void FolderScene::onEnd() {
  delete font;
  delete chipFont;
  delete numberFont;
  delete menuLabel;
  delete numberLabel;
}