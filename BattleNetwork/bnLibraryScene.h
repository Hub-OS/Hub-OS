#pragma once
#include <Swoosh/Ease.h>
#include <Swoosh/Activity.h>

#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnChip.h"
#include "bnAnimation.h"
#include "bnAnimatedTextBox.h"

#include <list>

class LibraryScene : public swoosh::Activity {
private:
  Camera camera;
  AnimatedTextBox textbox;

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

  // Chip description font
  sf::Font *chipDescFont;
  sf::Text* chipDesc;

  // folder menu graphic
  sf::Sprite bg;
  sf::Sprite folderDock;
  sf::Sprite scrollbar;
  sf::Sprite stars;
  sf::Sprite chipHolder;
  sf::Sprite element;
  sf::Sprite cursor;

  // Current chip graphic
  sf::Sprite chip;
  sf::IntRect cardSubFrame;

  sf::Sprite chipIcon;
  swoosh::Timer chipRevealTimer;
  swoosh::Timer easeInTimer;

  int maxChipsOnScreen;
  int currChipIndex;
  int lastChipOnScreen; // index
  int prevIndex; // for effect
  int numOfChips;

  double totalTimeElapsed;
  double frameElapsed;
  bool gotoNextScene;

  std::list<Chip> uniqueChips;

  void MakeUniqueChipsFromPack();

public:
  std::string FormatChipDesc(const std::string&& desc);

  virtual void onStart();
  virtual void onUpdate(double elapsed);
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onEnd();

  LibraryScene(swoosh::ActivityController&);
  virtual ~LibraryScene();
};
