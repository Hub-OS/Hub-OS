#pragma once
#include "Swoosh/Activity.h"

#include "bnFolderScene.h"
#include "bnOverworldMap.h"
#include "bnInfiniteMap.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnLibraryScene.h"
#include "bnFolderScene.h"
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
#include "bnChipFolderCollection.h"
#include <SFML/Graphics.hpp>
#include <time.h>

class MainMenuScene : public swoosh::Activity {
private:
  Camera camera;
  bool showHUD;

  // Selection input delays
  double maxSelectInputCooldown;
  double selectInputCooldown;

  // ui sprite maps
  sf::Sprite ui;
  Animation uiAnimator;

  int menuSelectionIndex;;

  sf::Sprite overlay;
  sf::Sprite ow;

  Background* bg;
  Overworld::Map* map;

  SelectedNavi currentNavi;
  sf::Sprite owNavi;
  Animation naviAnimator;

  bool gotoNextScene;

  ChipFolderCollection data; // TODO: this will be replaced with all saved data
public:
  MainMenuScene(swoosh::ActivityController&);
  virtual void onUpdate(double elapsed);
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onStart();
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onEnd();
  virtual ~MainMenuScene() { ; }
};
