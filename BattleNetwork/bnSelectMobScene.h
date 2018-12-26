#pragma once

#include "bnNaviRegistration.h"
#include "bnTextBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnStarman.h"
#include "bnMob.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnBattleScene.h"
#include "bnMobFactory.h"
#include "bnRandomMettaurMob.h"
#include "bnProgsManBossFight.h"
#include "bnTwoMettaurMob.h"
#include "bnCanodumbMob.h"

#include <time.h>
#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

class SelectMobScene : public swoosh::Activity
{
private:
  SelectedNavi selectedNavi;

  Camera camera;

  // Menu name font
  sf::Font* font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown;
  double selectInputCooldown;

  // MOB UI font
  sf::Font *mobFont;
  sf::Text *mobLabel;

  sf::Text *attackLabel;

  sf::Text *speedLabel;

  sf::Font *hpFont;
  sf::Text *hpLabel;

  float maxNumberCooldown;
  float numberCooldown;

  // select menu graphic
  sf::Sprite bg;

  // Current mob graphic
  sf::Sprite mob;

  // Animator for mobs
  Animation mobAnimator;

  // Transition
  sf::Shader& transition;
  float transitionProgress;

  bool gotoNextScene;

  SmartShader shader;
  float factor;

  // Current selection index
  int mobSelectionIndex;

  // Text box navigator
  TextBox textbox;

public:
  SelectMobScene(swoosh::ActivityController&, SelectedNavi);
  ~SelectMobScene() { ; }

  virtual void onResume();
  virtual void onUpdate(double elapsed);
};

