#include "bnMobBattleScene.h"
#include "../bnMob.h"

#include "States/bnRewardBattleState.h"
#include "States/bnTimeFreezeBattleState.h"
#include "States/bnBattleStartBattleState.h"
#include "States/bnBattleOverBattleState.h"
#include "States/bnFadeOutBattleState.h"
#include "States/bnCombatBattleState.h"
#include "States/bnCharacterTransformBattleState.h"
#include "States/bnMobIntroBattleState.h"
#include "States/bnCardSelectBattleState.h"
#include "States/bnCardComboBattleState.h"

using TrackedFormData = CharacterTransformBattleState::TrackedFormData;
using namespace swoosh;

MobBattleScene::MobBattleScene(ActivityController& controller, const MobBattleProperties& props)
: props(props), BattleSceneBase(controller, props.base) {

  Mob* current = props.mobs.at(0);

  // Simple error reporting if the scene was not fed any mobs
  if (props.mobs.size() == 0) {
    Logger::Log(std::string("Warning: Mob list was empty when battle started."));
  }
  else if (current->GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Current mob was empty when battle started. Mob Type: ") + typeid(current).name());
  }
  else {
    LoadMob(*props.mobs.front());
  }

  // If playing co-op, add more players to track here
  std::vector<Player*> players = { &props.base.player };

  // ptr to player, form select index (-1 none), if should transform
  // TODO: just make this a struct to use across all states that need it...
  std::vector<std::shared_ptr<TrackedFormData>> trackedForms = { 
    std::make_shared<TrackedFormData>(std::make_tuple(&props.base.player, -1, false))
  }; 

  // in seconds
  double battleDuration = 10.0;

  // First, we create all of our scene states
  auto intro       = AddState<MobIntroBattleState>(current, players);
  auto cardSelect  = AddState<CardSelectBattleState>(players, trackedForms);
  auto combat      = AddState<CombatBattleState>(current, players, battleDuration);
  auto combo       = AddState<CardComboBattleState>(this->GetSelectedCardsUI(), props.base.programAdvance);
  auto forms       = AddState<CharacterTransformBattleState>(trackedForms);
  auto battlestart = AddState<BattleStartBattleState>(players);
  auto battleover  = AddState<BattleOverBattleState>();
  auto timeFreeze  = AddState<TimeFreezeBattleState>();
  auto reward      = AddState<RewardBattleState>(current, &props.base.player);
  auto fadeout     = AddState<FadeOutBattleState>(FadeOut::black); // this state requires arguments

  //        hange from        ,to          ,when this is true
  CHANGE_ON_EVENT(intro       ,cardSelect  ,IsOver);
  CHANGE_ON_EVENT(cardSelect  ,forms       ,HasForm);
  CHANGE_ON_EVENT(cardSelect  ,battlestart ,OKIsPressed);
  CHANGE_ON_EVENT(forms       ,battlestart ,IsFinished);
  CHANGE_ON_EVENT(battlestart ,combat      ,IsFinished);
  CHANGE_ON_EVENT(battleover  ,reward      ,IsFinished);
  CHANGE_ON_EVENT(timeFreeze  ,combat      ,IsOver);
  CHANGE_ON_EVENT(reward      ,fadeout     ,OKIsPressed);

  // combat has multiple state interruptions based on events
  // so we chain them together instead of using the macro
  combat  .ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
          .ChangeOnEvent(fadeout,    &CombatBattleState::PlayerLost)
          .ChangeOnEvent(cardSelect, &CombatBattleState::PlayerRequestCardSelect)
          .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(intro);
}

MobBattleScene::~MobBattleScene() {

}

void MobBattleScene::onStart()
{
  //LoadMob(*props.mobs.front());
  BattleSceneBase::onStart();
}

void MobBattleScene::onExit()
{
}

void MobBattleScene::onEnter()
{
}

void MobBattleScene::onResume()
{
}

void MobBattleScene::onEnd()
{
}
