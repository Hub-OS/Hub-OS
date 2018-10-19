#pragma once
#include "bnActivity.h"
#include "bnFolderScene.h"
#include "bnOverworldMap.h"
#include "bnInfiniteMap.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include <SFML/Graphics.hpp>
#include <time.h>

class MainMenuScene : public Activity {
private:
  Camera camera;
  bool showHUD;

  // Selection input delays
  double maxSelectInputCooldown;
  double selectInputCooldown;

  // ui sprite maps
  sf::Sprite ui;
  Animation uiAnimator;


  // Transition
  sf::Shader* transition;
  float transitionProgress;

  sf::Clock clock;
  float elapsed;
  float totalTime;
  int menuSelectionIndex;;

  sf::Sprite overlay;
  sf::Sprite ow;

  Background* bg;
  Overworld::Map* map;

  SelectedNavi currentNavi;
  sf::Sprite owNavi;
  Animation naviAnimator;

  bool gotoNextScene;

public:
  MainMenuScene() = default;
  virtual void OnStart();
  virtual void OnUpdate(double _elapsed);
  virtual void OnLeave();
  virtual void OnResume();
  virtual void OnDraw(sf::RenderTexture& surface);
  virtual void OnEnd();
  virtual ~MainMenuScene() { ; }
};
