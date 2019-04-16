#pragma once
#include <time.h>

#include "bnSelectMobScene.h"
#include "bnNaviRegistration.h"
#include "bnGridBackground.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnAnimation.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnTextBox.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#define UI_LEFT_POS_MAX 10.f
#define UI_RIGHT_POS_MAX 300.f
#define UI_TOP_POS_MAX 0.f
#define UI_SPACING 55.f

#define UI_LEFT_POS_START -300.f
#define UI_RIGHT_POS_START 640.f
#define UI_TOP_POS_START 250.f

#define MAX_PIXEL_FACTOR 125.f

class SelectNaviScene : public swoosh::Activity
{
private:
  SelectedNavi& naviSelectionIndex;
  SelectedNavi prevChosen;

  Camera camera;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;

  // NAVI UI font
  sf::Font* font;
  sf::Font *naviFont;
  sf::Text* menuLabel;
  sf::Text *naviLabel;
  sf::Text *attackLabel;
  sf::Text *speedLabel;
  sf::Text *hpLabel;

  float maxNumberCooldown;
  float numberCooldown; // half a second

  // select menu graphic
  Background* bg;

  // UI Sprites
  float UI_RIGHT_POS;
  float UI_LEFT_POS;
  float UI_TOP_POS;

  sf::Sprite charName;
  sf::Sprite charElement;
  sf::Sprite charStat;
  sf::Sprite charInfo;
  sf::Sprite element;

  // Current navi graphic
  bool loadNavi;

  sf::Sprite navi;
  // Animator for navi
  Animation naviAnimator;

  // Distortion effect
  double factor;

  float transitionProgress;

  bool gotoNextScene;

  SmartShader pixelated;

  // Load glowing pad animation (never changes/always plays)
  Animation glowpadAnimator;
  sf::Sprite glowpad;
  sf::Sprite glowbase;
  sf::Sprite glowbottom;

  // Text box 
  TextBox textbox;

  double elapsed;
public:
  SelectNaviScene(swoosh::ActivityController& controller, SelectedNavi& navi);
  virtual ~SelectNaviScene();

  virtual void onUpdate(double elapsed);
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onStart();
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onEnd();
};

