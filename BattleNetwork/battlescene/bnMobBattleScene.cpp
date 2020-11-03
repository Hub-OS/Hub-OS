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
    std::make_shared<TrackedFormData>(TrackedFormData{&props.base.player, -1, false})
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
  auto battleover  = AddState<BattleOverBattleState>(players);
  auto timeFreeze  = AddState<TimeFreezeBattleState>();
  auto reward      = AddState<RewardBattleState>(current, &props.base.player);
  auto fadeout     = AddState<FadeOutBattleState>(FadeOut::black, players); // this state requires arguments

  // Important! State transitions are added in order of priority!
  intro.ChangeOnEvent(cardSelect, &MobIntroBattleState::IsOver);
  cardSelect.ChangeOnEvent(combo,  &CardSelectBattleState::OKIsPressed);
  combo.ChangeOnEvent(forms, [cardSelect, combo]() mutable {return combo->IsDone() && cardSelect->HasForm(); });
  combo.ChangeOnEvent(battlestart, &CardComboBattleState::IsDone);
  forms.ChangeOnEvent(combat, &CharacterTransformBattleState::Decrossed);
  forms.ChangeOnEvent(battlestart, &CharacterTransformBattleState::IsFinished);
  battlestart.ChangeOnEvent(combat, &BattleStartBattleState::IsFinished);
  battleover.ChangeOnEvent(reward, &BattleOverBattleState::IsFinished);
  timeFreeze.ChangeOnEvent(combat, &TimeFreezeBattleState::IsOver);

  // share some values between states
  combo->ShareCardList(&cardSelect->GetCardPtrList(), &cardSelect->GetCardListLengthAddr());

  // special condition: if lost in combat and had a form, trigger the character transform states
  auto playerLosesInForm = [trackedForms] {
    const bool changeState = trackedForms[0]->player->GetHealth() == 0 && (trackedForms[0]->selectedForm != -1);

    if (changeState) {
      trackedForms[0]->selectedForm = -1;
      trackedForms[0]->animationComplete = false;
    }

    return changeState;
  };

  // combat has multiple state interruptions based on events
  // so we can chain them together
  combat  .ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
          .ChangeOnEvent(forms,      playerLosesInForm)
          .ChangeOnEvent(fadeout,    &CombatBattleState::PlayerLost)
          .ChangeOnEvent(cardSelect, &CombatBattleState::PlayerRequestCardSelect)
          .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

  // Some states need to know about card uses
  auto& ui = this->GetSelectedCardsUI();
  combat->Subscribe(ui);
  timeFreeze->Subscribe(ui);
  
  // Some states are part of the combat routine and need to respect
  // the combat state's timers
  combat->subcombatStates.push_back(&timeFreeze.Unwrap());

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(intro);
}

MobBattleScene::~MobBattleScene() {
  for (auto&& m : props.mobs) {
    // delete m;
  }

  props.mobs.clear();
}

void MobBattleScene::onStart()
{
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

void MobBattleScene::onLeave()
{
  BattleSceneBase::onLeave();
}

void MobBattleScene::onEnd()
{
}
