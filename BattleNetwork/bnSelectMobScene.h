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

#include "Swoosh\Activity.h"
#include "Segues\CrossZoom.h"
#include "Segues\BlackWashFade.h"

class SelectMobScene : public swoosh::Activity
{
private:
  SelectedNavi selectedNavi;

  Camera camera;

  MobFactory* factory;
  Mob* mob;
  Field* field;

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

  bool doOnce; // scene effects
  bool showMob; // toggle hide / show mob

  // select menu graphic
  sf::Sprite bg;

  // Current mob graphic
  sf::Sprite mobSpr;

  // Animator for mobs
  Animation mobAnimator;

  sf::Sprite navigator;
  Animation navigatorAnimator;

  bool gotoNextScene;

  SmartShader shader;
  float factor;

  // Current selection index
  int mobSelectionIndex;

  // Text box navigator
  TextBox textbox;

public:
  SelectMobScene(swoosh::ActivityController&, SelectedNavi);
  ~SelectMobScene();

  virtual void onResume();
  virtual void onUpdate(double elapsed);
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onStart();
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onEnd();
};

