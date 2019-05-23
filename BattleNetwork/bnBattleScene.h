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
 * 
 * At the beginning it loads the mob and then sets the state to chip select. 
 * After, the pre-battle begin state shows before the player and mobs are free to move around.
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
  PA programAdvance; /*!< PA object loads PA database and returns matching PA chip from input */
  PASteps paSteps; /*!< Matching steps in a PA */
  bool isPAComplete; /*!< Flag if PA state is complete */
  int hasPA; /*!< If -1, no PA found, otherwise is the start of the PA in the current chip list */
  int paStepIndex; /*!< Index in the PA list */

  float listStepCooldown; /*!< Remaining time inbetween PA list items */
  float listStepCounter; /*!< Max time inbetween PA list items */
 
  sf::Sprite programAdvanceSprite; /*!< Sprite for "ProgramAdvanced" graphic */

  /*
  Mob labels*/
  std::vector<std::string> mobNames; /*!< List of every non-deleted mob spawned */

  Camera camera; /*!< Camera object - will shake screen */

  /*
  Other battle labels
  */

  sf::Sprite battleStart; /*!< "Battle Start" graphic */
  sf::Sprite battleEnd;   /*!< "Enemy Deleted" graphic */
  sf::Sprite doubleDelete; /*!< "Double Delete" graphic */
  sf::Sprite tripleDelete; /*!< "Triple Delete" graphic */
  sf::Sprite counterHit; /*!< "Counter Hit" graphic */
  sf::Sprite comboInfo;  /*!< double delete and triple delete placeholder. Only one appears at a time */
  sf::Vector2f comboInfoPos; /*!< Position of comboInfo on screen */
  sf::Vector2f battleStartPos; /*!< Position of battle pre/post graphic on screen */
  swoosh::Timer comboInfoTimer; /*!< How long the info should stay on screen */
  swoosh::Timer battleStartTimer; /*!< How long the start graphic should stay on screen */
  swoosh::Timer battleEndTimer; /*!< How long the end graphic should stay on screen */

  /*
  Chips + Chip select setup*/
  ChipSelectionCust chipCustGUI; /*!< Chip selection GUI that has an API to interact with */
  Chip** chips; /*!< List of Chip* the user selects from the chip cust */
  int chipCount; /*!< Length of chip list */

  /*
  Chip Summons*/
  ChipSummonHandler summons; /*!< TFC sytem */

  /*
  Battle results pointer */
  BattleResults* battleResults; /*!< BR modal that pops up when player wins */
  swoosh::Timer battleTimer; /*!< Total duration of active battle time */
  swoosh::Timer PAStartTimer; /*!< Time to scale the PA graphic */
  double PAStartLength; /*!< Total time to animate PA */

  /*
  Set Scene*/
  Field* field; /*!< Supplied by mob info: the grid to battle on */

  Player* player; /*!< Pointer to player's selected character */

  // Chip UI for player
  SelectedChipsUI chipUI; /*!< Player's Chip UI implementation */
  PlayerChipUseListener chipListener; /*!< Chip use listener handles one chip at a time */ 
  EnemyChipUseListener enemyChipListener; /*!< Enemies can use chips now */

  // TODO: replace with persistent storage object?
  ChipFolder* persistentFolder; /*!< Add rewarded items to the player's folder */

  // Other components
  std::vector<Component*> components; /*!< Components injected into the scene */

  /*
  Background for scene*/
  Background* background; /*!< Custom backgrounds provided by Mob data */

  int randBG; /*!< If background provided by Mob data is nullptr, randomly select one */

  // PAUSE
  sf::Font* font; /*!< PAUSE font */
  sf::Text* pauseLabel; /*!< "PAUSE" test */

  // CHIP CUST GRAPHICS
  sf::Texture* customBarTexture; /*!< Cust gauge image */
  SpriteSceneNode customBarSprite; /*!< Cust gauge sprite */
  sf::Vector2f customBarPos; /*!< Cust gauge position */

  // Selection input delays
  double maxChipSelectInputCooldown; /*!< When interacting with Chip Cust GUI API, delay input */
  double chipSelectInputCooldown; /*!< Time remaining with delayed input */

  // MOB
  sf::Font *mobFont; /*!< Name of mob font */
  Mob* mob; /*!< Mob and mob data player are fighting against */

  // States. TODO: Abstract this further into battle state classes
  bool isPaused; /*!< Pause state */
  bool isInChipSelect; /*!< In chip cust GUI state */
  bool isChipSelectReady; /*!< Used to interact with chip cust GUI, if selected chips are ready to use */
  bool isPlayerDeleted; /*!< If player is deleted this frame */
  bool isMobDeleted; /*!< If the entire mob is deleted this frame */
  bool isBattleRoundOver; /*!< Is round over state */
  bool isMobFinished; /*!< Is mob finished spawning state */
  bool isSceneInFocus; /*!< Is the Activity in focus. Has OnStart() been called. */
  bool prevSummonState; /*!< Flag to monitor if TFC has ended or began */
  double customProgress; /*!< Cust bar progress */
  double customDuration; /*!< Cust bar max time */
  bool initFadeOut; /*!< Fade out white state. Used when player dies */
  bool didDoubleDelete; /*!< Flag if player double deleted this frame */
  bool didTripleDelete; /*!< Flag if player tripled deleted this frame */

  bool isPreBattle; /*!< Is in pre-battle "Battle Start" state */
  bool isPostBattle; /*!< Is in post-battle "Enemy Deleted" state */
  double preBattleLength; /*!< Duration of state */
  double postBattleLength; /*!< Duration of state */

  int lastMobSize; /*!< used to determine double/triple deletes with frame accuracy */
  int totalCounterMoves; /*!< Track player's counters. Used for ranking. */
  int totalCounterDeletions; /*!< Track player's counter-deletions. Used for ranking. */

  double summonTimer; /*!< Timer for TFC label to appear at top */
  bool showSummonText; /*!< Whether or not TFC label should appear */
  double summonTextLength; /*!< How long TFC label should stay on screen */

  // Special: Load shaders if supported 
  double shaderCooldown;

  sf::Shader& pauseShader; /*!< Dim screen */
  sf::Shader& whiteShader; /*!< Fade out white */
  sf::Shader& yellowShader; /*!< Turn tiles yellow */
  sf::Shader& customBarShader; /*!< Cust gauge shaders */
  sf::Shader& heatShader; /*!< Heat waves and red hue */ 
  sf::Shader& iceShader; /*!< Reflection in the ice */

  // Heat distortion effect
  sf::Texture& distortionMap; /*!< Distortion effect pixel sample source */
  sf::Vector2u textureSize; /*!< Size of distorton effect */

  //graphics that appear onscreen
  std::vector<SceneNode*> scenenodes; /*!< Scene node system */

  // for time-based graphics effects
  double elapsed; /*!< total time elapsed in battle */

  /**
   * @brief Get the total number of counter moves
   * @return const int
   */
  const int GetCounterCount() const {
    return totalCounterMoves;
  }

  /**
   * @brief The scene is registered as a counter subscriber to listen for counter events
   * @param victim who was countered
   * @param aggressor who caused the counter
   */
  virtual void OnCounter(Character& victim, Character& aggressor);

public:

  /**
   * @brief Update scene during proper state 
   * @param elapsed in seconds
   * 
   * Currently uses boolean flags for state management
   */
  virtual void onUpdate(double elapsed);
  
  /**
   * @brief Draw scene
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief When scene is in focus (when transition ends)
   */
  virtual void onStart();
  
  /**
   * @brief When scene loses focus (when exit transition begins)
   */
  virtual void onLeave();
  
  /**
   * @brief When scene is out of focus completely (exit transition ended)
   */
  virtual void onExit();
  
  /**
   * @brief When scene gains focus (when enter transition begins)
   */
  virtual void onEnter();
  
  /**
   * @brief When scene is in focus after having lost it once before
   */
  virtual void onResume();
  
  /**
   * @brief When scene is terminated
   */
  virtual void onEnd();
  
  /**
   * @brief Construct scene with selected player, generated mob data, and the folder to use
   */
  BattleScene(swoosh::ActivityController&, Player*, Mob*, ChipFolder* folder);
  
  /**
   * @brief Clears all nodes and components
   */
  virtual ~BattleScene();

  /**
   * @brief Inject uses double-visitor design pattern. Battle Scene subscribes to chip pub components.
   * @param pub ChipUsePublisher component to subscribe to
   */
  void Inject(ChipUsePublisher& pub);
  
  /**
   * @brief Inject uses double-visitor design pattern. BattleScene adds component to draw and update list.
   * @param other
   */
  void Inject(MobHealthUI & other);
  
  /**
   * @brief Inject uses double-visitor design pattern. This is default case.
   * @param other Adds component "other" to component update list.
   */
  void Inject(Component* other);
  
  /**
   * @brief When ejecting component from scene, simply removes it from update list
   * @param other
   */
  void Eject(Component* other);

  /**
   * @brief Scans the entity list for updated components and tries to Inject them if the components require.
   */
  void ProcessNewestComponents();

  /**
   * @brief State boolean for BattleScene. Query if the battle is over.
   * @return true if isPostBattle is true, otherwise false
   */
  const bool IsCleared() {
    return isPostBattle;
  }
};
