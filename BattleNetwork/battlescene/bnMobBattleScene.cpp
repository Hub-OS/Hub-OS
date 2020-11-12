#include "bnMobBattleScene.h"
#include "../bnMob.h"
#include "../bnElementalDamage.h"

#include "States/bnRewardBattleState.h"
#include "States/bnTimeFreezeBattleState.h"
#include "States/bnBattleStartBattleState.h"
#include "States/bnBattleOverBattleState.h"
#include "States/bnFadeOutBattleState.h"
#include "States/bnCombatBattleState.h"
#include "States/bnMobIntroBattleState.h"
#include "States/bnCardSelectBattleState.h"
#include "States/bnCardComboBattleState.h"

using namespace swoosh;

MobBattleScene::MobBattleScene(ActivityController& controller, const MobBattleProperties& props) : 
  props(props), 
  BattleSceneBase(controller, props.base) {

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
  players = { &props.base.player };

  // ptr to player, form select index (-1 none), if should transform
  // TODO: just make this a struct to use across all states that need it...
  trackedForms = { 
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
  auto reward      = AddState<RewardBattleState>(current, &props.base.player, &playerHitCount);
  auto fadeout     = AddState<FadeOutBattleState>(FadeOut::black, players); // this state requires arguments

  // Important! State transitions are added in order of priority!
  intro.ChangeOnEvent(cardSelect, &MobIntroBattleState::IsOver);
  cardSelect.ChangeOnEvent(combo,  &CardSelectBattleState::OKIsPressed);
  combo.ChangeOnEvent(forms, [cardSelect, combo]() mutable {return combo->IsDone() && cardSelect->HasForm(); });
  combo.ChangeOnEvent(battlestart, &CardComboBattleState::IsDone);
  forms.ChangeOnEvent(combat, [forms, cardSelect, this]() mutable { 
    bool triggered = forms->IsFinished() && (GetPlayer()->GetHealth() == 0 || playerDecross); 

    if (triggered) {
      playerDecross = false; // reset our decross flag

      // update the card select gui and state
      // since the state has its own records
      cardSelect->ResetSelectedForm();
    }

    return triggered; 
  });
  forms.ChangeOnEvent(battlestart, &CharacterTransformBattleState::IsFinished);
  battlestart.ChangeOnEvent(combat, &BattleStartBattleState::IsFinished);
  battleover.ChangeOnEvent(reward, &BattleOverBattleState::IsFinished);
  timeFreeze.ChangeOnEvent(combat, &TimeFreezeBattleState::IsOver);

  // share some values between states
  combo->ShareCardList(&cardSelect->GetCardPtrList(), &cardSelect->GetCardListLengthAddr());

  // special condition: if in combat and should decross, trigger the character transform states
  auto playerDecrosses = [this, forms] () mutable {
    bool changeState = this->trackedForms[0]->player->GetHealth() == 0;
    changeState = changeState || playerDecross;
    changeState = changeState && (this->trackedForms[0]->selectedForm != -1);

    if (changeState) {
      this->trackedForms[0]->selectedForm = -1;
      this->trackedForms[0]->animationComplete = false;
      forms->SkipBackdrop();
    }

    return changeState;
  };

  // combat has multiple state interruptions based on events
  // so we can chain them together
  combat  .ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
          .ChangeOnEvent(forms,      playerDecrosses)
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
  combat->subcombatStates.push_back(&forms.Unwrap());

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(intro);
}

MobBattleScene::~MobBattleScene() {
  for (auto&& m : props.mobs) {
    // delete m;
  }

  props.mobs.clear();
}

void MobBattleScene::OnHit(Character& victim, const Hit::Properties& props)
{
  if (GetPlayer() == &victim && props.damage > 0) {
    playerHitCount++;

    if (GetPlayer()->IsSuperEffective(props.element)) {
      playerDecross = true;
    }
  }

  if (victim.IsSuperEffective(props.element) && props.damage > 0) {
    Artifact* seSymbol = new ElementalDamage(GetField());
    seSymbol->SetLayer(-100);
    GetField()->AddEntity(*seSymbol, victim.GetTile()->GetX(), victim.GetTile()->GetY());
  }
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
