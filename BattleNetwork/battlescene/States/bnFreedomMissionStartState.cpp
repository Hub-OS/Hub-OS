#include "bnFreedomMissionStartState.h"

#include "../../bnPlayer.h"
#include "../bnBattleSceneBase.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"
#include "../../bnPlayerSelectedCardsUI.h"

FreedomMissionStartState::FreedomMissionStartState(uint8_t maxTurns) :
  maxTurns(maxTurns),
  BattleTextIntroState() {
}

void FreedomMissionStartState::onStart(const BattleSceneState* _)
{
  BattleTextIntroState::onStart(_);

  GetScene().IncrementTurnCount();
  const uint8_t turnCount = GetScene().GetTurnCount();

  if (maxTurns == turnCount) {
    SetIntroText("Final Turn!");
    return;
  } 

  SetIntroText("Round " + std::to_string(turnCount) + " Start!");

  // only reveal first player's UI widget to them
  auto ui = GetScene().GetLocalPlayer()->GetFirstComponent<PlayerSelectedCardsUI>();

  if (ui) {
    ui->Reveal();
  }
}