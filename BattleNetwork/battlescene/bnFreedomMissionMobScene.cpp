#include "bnFreedomMissionMobScene.h"
#include "../bnMob.h"
#include "../bnElementalDamage.h"
#include "../../bnPlayer.h"

#include "States/bnTimeFreezeBattleState.h"
#include "States/bnFreedomMissionStartState.h"
#include "States/bnFreedomMissionOverState.h"
#include "States/bnFadeOutBattleState.h"
#include "States/bnCombatBattleState.h"
#include "States/bnMobIntroBattleState.h"
#include "States/bnCardSelectBattleState.h"
#include "States/bnCardComboBattleState.h"
#include "../bnBlockPackageManager.h"

using namespace swoosh;

FreedomMissionMobScene::FreedomMissionMobScene(ActivityController& controller, FreedomMissionProps _props, BattleResultsFunc onEnd) :
  BattleSceneBase(controller, _props.base, onEnd),
  props(std::move(_props))
{

  Init();

  Mob* mob = props.mobs.at(0);

  // in seconds
  constexpr double battleDuration = 10.0;

  // First, we create all of our scene states
  auto intro       = AddState<MobIntroBattleState>(mob);
  auto cardSelect  = AddState<CardSelectBattleState>();
  auto combat      = AddState<CombatBattleState>(mob, battleDuration);
  auto combo       = AddState<CardComboBattleState>(this->GetSelectedCardsUI(), props.base.programAdvance);
  auto forms       = AddState<CharacterTransformBattleState>();
  auto battlestart = AddState<FreedomMissionStartState>(props.maxTurns);
  auto battleover  = AddState<FreedomMissionOverState>();
  auto timeFreeze  = AddState<TimeFreezeBattleState>();
  auto fadeout     = AddState<FadeOutBattleState>(FadeOut::black);

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
  battlestart.ChangeOnEvent(combat, &FreedomMissionStartState::IsFinished);

  // Mob rewards players after the battle over animations are finished
  battleover.ChangeOnEvent(fadeout, &FreedomMissionOverState::IsFinished);

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

  auto cardGaugeIsFull = [this]() mutable {
    return GetCustomBarProgress() >= GetCustomBarDuration();
  };

  auto outOfTurns = [this]() mutable {
    if (GetCustomBarProgress() >= GetCustomBarDuration() && GetTurnCount() == FreedomMissionMobScene::props.maxTurns) {
      overStatePtr->context = FreedomMissionOverState::Conditions::player_failed;
      return true;
    }

    return false;
  };

  // combat has multiple state interruptions based on events
  // so we can chain them together
  combat.ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
    .ChangeOnEvent(battleover, &CombatBattleState::PlayerLost)
    .ChangeOnEvent(battleover, outOfTurns)
    .ChangeOnEvent(cardSelect, cardGaugeIsFull)
    .ChangeOnEvent(forms, playerDecrosses)
    .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

  // Time freeze state interrupts combat state which must resume after animations are finished
  timeFreeze.ChangeOnEvent(combat, &TimeFreezeBattleState::IsOver);

  overStatePtr = &battleover.Unwrap();

  // Some states are part of the combat routine and need to respect
  // the combat state's timers
  combat->subcombatStates.push_back(&timeFreeze.Unwrap());
  combat->subcombatStates.push_back(&forms.Unwrap());

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(intro);
}

FreedomMissionMobScene::~FreedomMissionMobScene() {
  for (auto&& m : props.mobs) {
    delete m;
  }

  props.mobs.clear();
}

void FreedomMissionMobScene::Init()
{
  SpawnLocalPlayer(2, 2);

  // Run block programs on the remote player now that they are spawned
  BlockPackageManager& blockPackages = getController().BlockPackagePartitioner().GetPartition(Game::LocalPartition);
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

  auto& cardSelectWidget = GetCardSelectWidget();
  cardSelectWidget.PreventRetreat();
  cardSelectWidget.SetSpeaker(props.mug, props.anim);
  GetEmotionWindow().SetTexture(props.emotion);

  // Tell everything to begin battle
  BroadcastBattleStart();
}

void FreedomMissionMobScene::OnHit(Entity& victim, const Hit::Properties& props)
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

void FreedomMissionMobScene::onUpdate(double elapsed)
{
  ProcessLocalPlayerInputQueue();
  BattleSceneBase::onUpdate(elapsed);
}

void FreedomMissionMobScene::onStart()
{
  BattleSceneBase::onStart();
}

void FreedomMissionMobScene::onExit()
{
}

void FreedomMissionMobScene::onEnter()
{
}

void FreedomMissionMobScene::onResume()
{
}

void FreedomMissionMobScene::onLeave()
{
  BattleSceneBase::onLeave();
}

void FreedomMissionMobScene::IncrementTurnCount()
{
  BattleSceneBase::IncrementTurnCount();

  if (GetTurnCount() == 1) {
    overStatePtr->context = FreedomMissionOverState::Conditions::player_won_single_turn;
  }
  else {
    overStatePtr->context = FreedomMissionOverState::Conditions::player_won_mutliple_turn;
  }
}