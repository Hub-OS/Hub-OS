#include "bnMobBattleScene.h"
#include "../bnMob.h"
#include "../bnElementalDamage.h"
#include "../../bnPlayer.h"

#include "States/bnRewardBattleState.h"
#include "States/bnTimeFreezeBattleState.h"
#include "States/bnBattleStartBattleState.h"
#include "States/bnBattleOverBattleState.h"
#include "States/bnFadeOutBattleState.h"
#include "States/bnCombatBattleState.h"
#include "States/bnMobIntroBattleState.h"
#include "States/bnCardSelectBattleState.h"
#include "States/bnCardComboBattleState.h"
#include "States/bnRetreatBattleState.h"
#include "../bnBlockPackageManager.h"

using namespace swoosh;

MobBattleScene::MobBattleScene(ActivityController& controller, MobBattleProperties _props, BattleResultsFunc onEnd) :
  BattleSceneBase(controller, _props.base, onEnd),
  props(std::move(_props))
{
  // Load players in the correct order, then the mob
  Init();

  Mob* current = props.mobs.at(0);

  // in seconds
  constexpr double battleDuration = 10.0;

  // First, we create all of our scene states
  auto intro       = AddState<MobIntroBattleState>(current);
  auto cardSelect  = AddState<CardSelectBattleState>();
  auto combat      = AddState<CombatBattleState>(current, battleDuration);
  auto combo       = AddState<CardComboBattleState>(this->GetSelectedCardsUI(), props.base.programAdvance);
  auto forms       = AddState<CharacterTransformBattleState>();
  auto battlestart = AddState<BattleStartBattleState>();
  auto battleover  = AddState<BattleOverBattleState>();
  auto timeFreeze  = AddState<TimeFreezeBattleState>();
  auto reward      = AddState<RewardBattleState>(current, &playerHitCount);
  auto fadeout     = AddState<FadeOutBattleState>(FadeOut::black);

  // TODO: create a textbox in the battle scene and supply it to the card select widget and other states...
  auto retreat     = AddState<RetreatBattleState>(GetCardSelectWidget().GetTextBox(), props.mug, props.anim);

  combatPtr = &combat.Unwrap();
  timeFreezePtr = &timeFreeze.Unwrap();

  std::shared_ptr<PlayerSelectedCardsUI> cardUI = GetLocalPlayer()->GetFirstComponent<PlayerSelectedCardsUI>();
  combatPtr->Subscribe(*cardUI);
  timeFreezePtr->Subscribe(*cardUI);

  // Subscribe to player's events
  combatPtr->Subscribe(*GetLocalPlayer());
  timeFreezePtr->Subscribe(*GetLocalPlayer());

  //////////////////////////////////////////////////////////////////
  // Important! State transitions are added in order of priority! //
  //////////////////////////////////////////////////////////////////

  auto isIntroOver = [this, intro, timeFreeze, combat]() mutable {
    if (intro->IsOver()) {
      // Mob's mutated at spawn may have card use publishers.
      // Share the scene's subscriptions at this point in time with
      // those substates.
      for (auto& publisher : this->GetCardActionSubscriptions()) {
        timeFreeze->Subscribe(publisher);
        combat->Subscribe(publisher);
      }
      return true;
    }

    return false;
  };

  intro.ChangeOnEvent(cardSelect, isIntroOver);

  // Prevent all other conditions if the player tried to retreat
  cardSelect.ChangeOnEvent(retreat, &CardSelectBattleState::RequestedRetreat);

  // Goto the combo check state if new cards are selected...
  cardSelect.ChangeOnEvent(combo, &CardSelectBattleState::SelectedNewChips);

  // ... else if forms were selected, go directly to forms ....
  cardSelect.ChangeOnEvent(forms, &CardSelectBattleState::HasForm);

  // ... Finally if none of the above, just start the battle
  cardSelect.ChangeOnEvent(battlestart, &CardSelectBattleState::OKIsPressed);

  // If we reached the combo state, we must also check if form transformation was next
  // or to just start the battle after
  combo.ChangeOnEvent(forms, [cardSelect, combo]() mutable {return combo->IsDone() && cardSelect->HasForm(); });
  combo.ChangeOnEvent(battlestart, &CardComboBattleState::IsDone);

  // If retreating fails, we jump right into battle start
  retreat.ChangeOnEvent(battlestart, &RetreatBattleState::Fail);

  // Otherwise, we leave
  retreat.ChangeOnEvent(fadeout, [retreat, fadeout]() mutable {
    if (retreat->Success()) {
      // fadeout pauses animations only when retreating
      fadeout->EnableKeepPlaying(false);
      return true;
    }

    return false;
    }
  );

  // Forms is the last state before kicking off the battle
  // if we reached this state...
  forms.ChangeOnEvent(combat, [forms, cardSelect, this]() mutable { 
    bool triggered = forms->IsFinished() && (GetLocalPlayer()->GetHealth() == 0 || playerDecross); 

    if (triggered) {
      playerDecross = false; // reset our decross flag

      // update the card select gui and state
      // since the state has its own records
      cardSelect->ResetSelectedForm();
    }

    return triggered; 
  });
  forms.ChangeOnEvent(battlestart, &CharacterTransformBattleState::IsFinished);

  // Combat begins when the battle start animations are finished
  battlestart.ChangeOnEvent(combat, &BattleStartBattleState::IsFinished);

  // Mob rewards players after the battle over animations are finished
  battleover.ChangeOnEvent(reward, &BattleOverBattleState::IsFinished);

  // share some values between states
  combo->ShareCardList(cardSelect->GetCardPtrList());

  // special condition: if in combat and should decross, trigger the character transform states
  auto playerDecrosses = [this, forms] () mutable {
    std::shared_ptr<Player> localPlayer = GetLocalPlayer();
    TrackedFormData& formData = GetPlayerFormData(localPlayer);

    bool changeState = localPlayer->GetHealth() == 0;
    changeState = changeState || playerDecross;
    changeState = changeState && (formData.selectedForm != -1);

    if (changeState) {
      formData.selectedForm = -1;
      formData.animationComplete = false;
      forms->SkipBackdrop();
    }

    return changeState;
  };

  // combat has multiple state interruptions based on events
  // so we can chain them together
  combat.ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
    .ChangeOnEvent(forms, playerDecrosses)
    .ChangeOnEvent(fadeout, &CombatBattleState::PlayerLost)
    .ChangeOnEvent(cardSelect, &CombatBattleState::PlayerRequestCardSelect)
    .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

  // Time freeze state interrupts combat state which must resume after animations are finished
  timeFreeze.ChangeOnEvent(combat, &TimeFreezeBattleState::IsOver);
  
  // Some states are part of the combat routine and need to respect
  // the combat state's timers
  combat->subcombatStates.push_back(&timeFreeze.Unwrap());
  combat->subcombatStates.push_back(&forms.Unwrap());

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(intro);
}

MobBattleScene::~MobBattleScene() {
  for (auto&& m : props.mobs) {
    delete m;
  }

  props.mobs.clear();
}

void MobBattleScene::Init()
{
  SpawnLocalPlayer(2, 2);

  // Run block programs on the remote player now that they are spawned
  BlockPackageManager& blockPackages = getController().BlockPackageManager();
  for (const std::string& blockID : props.blocks) {
    if (!blockPackages.HasPackage(blockID)) continue;

    auto& blockMeta = blockPackages.FindPackageByID(blockID);
    blockMeta.mutator(*GetLocalPlayer());
  }

  Mob& mob = *props.mobs.front();

  // Simple error reporting if the scene was not fed any mobs
  if (props.mobs.size() == 0) {
    Logger::Log(LogLevel::warning, std::string("Mob list was empty when battle started."));
  }
  else if (mob.GetMobCount() == 0) {
    Logger::Log(LogLevel::warning, std::string("Current mob was empty when battle started. Mob Type: ") + typeid(mob).name());
  }
  else {
    LoadMob(mob);
  }

  GetCardSelectWidget().SetSpeaker(props.mug, props.anim);
  GetEmotionWindow().SetTexture(props.emotion);

  // Tell everything to begin battle
  BroadcastBattleStart();
}

void MobBattleScene::OnHit(Entity& victim, const Hit::Properties& props)
{
  auto player = GetLocalPlayer();
  if (player.get() == &victim && props.damage > 0) {
    playerHitCount++;

    if (props.damage >= 300) {
      player->SetEmotion(Emotion::angry);
      GetSelectedCardsUI().SetMultiplier(2);
    }

    if (player->IsSuperEffective(props.element)) {
      playerDecross = true;
    }
  }

  if (victim.IsSuperEffective(props.element) && props.damage > 0) {
    auto seSymbol = std::make_shared<ElementalDamage>();
    seSymbol->SetLayer(-100);
    seSymbol->SetHeight(victim.GetHeight()+(victim.getLocalBounds().height*0.5f)); // place it at sprite height
    GetField()->AddEntity(seSymbol, victim.GetTile()->GetX(), victim.GetTile()->GetY());
  }
}

void MobBattleScene::onUpdate(double elapsed)
{
  ProcessLocalPlayerInputQueue();
  BattleSceneBase::onUpdate(elapsed);
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
