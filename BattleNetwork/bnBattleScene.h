#pragma once

#include "Swoosh/Activity.h"
#include "Swoosh/Ease.h"
#include "Swoosh/Timer.h"

#include "bnMettaur.h"
#include "bnProgsMan.h"
#include "bnBackground.h"
#include "bnLanBackground.h"
#include "bnGraveyardBackground.h"
#include "bnVirusBackground.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnChipSelectionCust.h"
#include "bnChipFolder.h"
#include "bnShaderResourceManager.h"
#include "bnPA.h"
#include "bnEngine.h"
#include "bnSceneNode.h"
#include "bnBattleResults.h"
#include "bnBattleScene.h"
#include "bnMob.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnSelectedChipsUI.h"
#include "bnPlayerChipUseListener.h"
#include "bnEnemyChipUseListener.h"
#include "bnCounterHitListener.h"
#include "bnChipSummonHandler.h"
#include "bnMemory.h"

#include <time.h>
#include <typeinfo>
#include <SFML/Graphics.hpp>

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

class Mob;
class Player;
class PlayerHealthUI;

class BattleScene : public swoosh::Activity, public CounterHitListener {
private:
  /*
  Program Advance + labels
  */

  PA programAdvance;
  PASteps paSteps;
  bool isPAComplete;
  int hasPA;
  int paStepIndex;

  float listStepCooldown;
  float listStepCounter;

  sf::Sprite programAdvanceSprite;

  /*
  Mob labels*/
  std::vector<std::string> mobNames;

  Camera camera;

  /*
  Other battle labels
  */

  sf::Sprite battleStart;
  sf::Sprite battleEnd;
  sf::Sprite doubleDelete;
  sf::Sprite tripleDelete;
  sf::Sprite counterHit;
  sf::Sprite comboInfo; // double delete and triple delete placeholder. Only one appears at a time!
  sf::Vector2f comboInfoPos;
  sf::Vector2f battleStartPos;
  swoosh::Timer comboInfoTimer;
  swoosh::Timer battleStartTimer;
  swoosh::Timer battleEndTimer;

  /*
  Chips + Chip select setup*/
  ChipSelectionCust chipCustGUI;
  Chip** chips;
  int chipCount;

  /*
  Chip Summons*/
  ChipSummonHandler summons;

  /*
  Battle results pointer */
  BattleResults* battleResults;
  swoosh::Timer battleTimer;
  swoosh::Timer PAStartTimer;
  double PAStartLength;

  /*
  Set Scene*/
  Field* field;

  Player* player;
  //PlayerHealthUI* playerHealthUI;

  // Chip UI for player
  SelectedChipsUI chipUI;
  PlayerChipUseListener chipListener;
  EnemyChipUseListener enemyChipListener;

  // Other components
  std::vector<Component*> components;

  /*
  Background for scene*/
  Background* background;

  int randBG;

  // PAUSE
  sf::Font* font;
  sf::Text* pauseLabel;

  // CHIP CUST GRAPHICS
  sf::Texture* customBarTexture;
  LayeredDrawable customBarSprite;
  sf::Vector2f customBarPos;

  // Selection input delays
  double maxChipSelectInputCooldown;
  double chipSelectInputCooldown;

  // MOB
  sf::Font *mobFont;
  Mob* mob;

  // States. TODO: Abstract this further into battle state classes
  bool isPaused;
  bool isInChipSelect;
  bool isChipSelectReady;
  bool isPlayerDeleted;
  bool isMobDeleted;
  bool isBattleRoundOver;
  bool isMobFinished;
  bool prevSummonState;
  double customProgress;
  double customDuration;
  bool initFadeOut;
  bool didDoubleDelete;
  bool didTripleDelete;

  bool isPreBattle;
  bool isPostBattle;
  double preBattleLength; 
  double postBattleLength;

  int lastMobSize; // used to determine double/triple deletes with frame accuracy
  int totalCounterMoves; 
  int totalCounterDeletions; // used for ranking

  swoosh::Timer summonTimer;
  bool showSummonText;
  double summonTextLength; // in seconds

  // Special: Load shaders if supported 
  double shaderCooldown;

  sf::Shader& pauseShader;
  sf::Shader& whiteShader;
  sf::Shader& yellowShader;
  sf::Shader& customBarShader;
  sf::Shader& heatShader;
  sf::Shader& iceShader;

  // Heat distortion effect
  sf::Texture& distortionMap;
  sf::Vector2u textureSize; 


  // TODO: replace misc components with SceneNodes completely
  // e.g. this->AddSceneNodes(nodes);
  std::vector<SceneNode*> scenenodes;

  std::vector<std::vector<Drawable*>> miscComponents;

  // for time-based graphics effects
  double elapsed;

  const int GetCounterCount() const {
    return totalCounterMoves;
  }

  virtual void OnCounter(Character& victim, Character& aggressor);

public:
  virtual void onUpdate(double elapsed);
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onStart();
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onEnd();
  BattleScene(swoosh::ActivityController&, Player*, Mob*, ChipFolder* folder);
  virtual ~BattleScene();

  // External component injection into the battle system
  void Inject(ChipUsePublisher& pub);

  const bool IsCleared() {
    return isPostBattle;
  }
};