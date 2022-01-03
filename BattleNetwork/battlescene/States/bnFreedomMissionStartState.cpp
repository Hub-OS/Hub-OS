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
  BattleSceneBase& scene = GetScene();
  BattleTextIntroState::onStart(_);

  // only reveal first player's UI widget to them
  std::shared_ptr<PlayerSelectedCardsUI> ui = scene.GetLocalPlayer()->GetFirstComponent<PlayerSelectedCardsUI>();

  if (ui) {
    ui->Reveal();
  }

  scene.IncrementTurnCount();
  const uint8_t turnCount = scene.GetTurnCount();

  if (maxTurns == turnCount) {
    SetIntroText("Final Turn!");
    return;
  } 

  SetIntroText("Round " + std::to_string(turnCount) + " Start!");
}