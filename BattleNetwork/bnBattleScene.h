#pragma once

#include <Swoosh/Activity.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

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

/**
 * @class BattleScene
 * @author mav
 * @date 14/05/19
 * @file bnBattleScene.h
 * @brief BattleScene features player against mobs and has many states and widgets
 * 
 * @warning This BattleScene desperately needs to be refactored into smaller states BatteSceneState.
 * 
 * This is a massive scene. It has many interactive modals and manages all the incoming and outgoing data
 * about the scene to the mobs and widgets on the screen. 
 * 
 * Currently it handles the various states with boolean flags.
 * At the beginning it loads the mob and then sets the state to chip select. 
 * After the pre-battle begin state shows before the player and mobs are free to move around.
 * The ChipSummonHandler has its own separate state system since summoned entities can update during TFC.
 * 
 * The scene also handles the keyboard interaction and uses the modal APIs to interact with.
 * 
 * This scene should be broken down into pieces such as:
 * BattleSceneLoseState
 * BattleSceneWinState
 * BattleScenePauseState
 * BattleSceneChipSelectState
 * BattleSceneIntroState
 * BattleSceneBeginState -> can then be reused to make BattleSceneTurnBegin<3> for 3rd turn
 * 
 * This will drastically clean the code up and allow for new custom states. 
 * Custom scenes could include beast-out mode state, dialog state for talking, damage counter state, etc.
 */
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

  // TODO: replace with persistent storage object?
  ChipFolder* persistentFolder;

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
  SpriteSceneNode customBarSprite;
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
  bool isSceneInFocus;
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

  double summonTimer;
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

  //graphics that appear onscreen
  std::vector<SceneNode*> scenenodes;

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
  void Inject(MobHealthUI & other);
  void Inject(Component* other);
  void Eject(Component* other);

  void ProcessNewestComponents();

  const bool IsCleared() {
    return isPostBattle;
  }
};
