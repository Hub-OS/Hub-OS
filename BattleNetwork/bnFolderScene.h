#pragma once
#include "Swoosh/Ease.h"
#include "Swoosh/Activity.h"

#include "bnCamera.h"
#include "bnChipFolderCollection.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"

class FolderScene : public swoosh::Activity {
private:
  Camera camera;
  ChipFolderCollection& collection;
  ChipFolder* folder;
  std::vector<std::string> folderNames;

  // Menu name font
  sf::Font* font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;

  // Chip UI font
  sf::Font *chipFont;
  sf::Text *chipLabel;

  sf::Font *numberFont;
  sf::Text *numberLabel;

  // folder menu graphic
  sf::Sprite bg;
  sf::Sprite folderBox;
  sf::Sprite folderCursor;
  sf::Sprite scrollbar;
  sf::Sprite folderOptions;
  sf::Sprite element;
  sf::Sprite cursor;
  sf::Sprite folderEquip;
  sf::Sprite chipIcon;

  Animation equipAnimation;
  Animation folderCursorAnimation;

  int currFolderIndex;
  int selectedFolderIndex;

  int maxChipsOnScreen;
  int currChipIndex;
  int numOfChips;

  double totalTimeElapsed;
  double frameElapsed;

  double folderOffsetX;

  bool gotoNextScene;

  int optionIndex;
  bool promptOptions;
  bool enterText;

public:
  virtual void onStart();
  virtual void onUpdate(double elapsed);
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onEnd();

  FolderScene(swoosh::ActivityController&, ChipFolderCollection&);
  virtual ~FolderScene();
};
