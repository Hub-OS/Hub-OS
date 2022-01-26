#include "bnNetworkCardWaitBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../battlescene/States/bnCardSelectBattleState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

NetworkCardWaitBattleState::NetworkCardWaitBattleState(std::shared_ptr<Player>& remotePlayer, NetworkBattleScene* scene) :
  remotePlayer(remotePlayer),
  scene(scene),
  cardSelectState(scene->cardStatePtr),
  NetworkWaitBattleState()
{
}

NetworkCardWaitBattleState::~NetworkCardWaitBattleState()
{
}

void NetworkCardWaitBattleState::onStart(const BattleSceneState* last)
{
  if (cardSelectState && last == cardSelectState) {
    // We have returned from the card select state to force a handshake and wait 
    scene->SendHandshakeSignal();
  }
}

void NetworkCardWaitBattleState::onEnd(const BattleSceneState* next)
{
  if (firstConnection) {
    GetScene().GetLocalPlayer()->ChangeState<PlayerControlledState>();
    
    if (remotePlayer) {
      remotePlayer->ChangeState<PlayerControlledState>();
    }

    firstConnection = false;
  }

  NetworkWaitBattleState::onEnd(next);
}

bool NetworkCardWaitBattleState::IsRemoteConnected()
{
  return firstConnection && scene->remoteState.remoteConnected;
}

bool NetworkCardWaitBattleState::SelectedNewChips()
{
  return cardSelectState && cardSelectState->SelectedNewChips() && IsReady();
}

bool NetworkCardWaitBattleState::NoConditions()
{
  return cardSelectState && cardSelectState->OKIsPressed() && IsReady();
}

bool NetworkCardWaitBattleState::HasForm()
{
  return cardSelectState && (cardSelectState->HasForm() || scene->remoteState.remoteChangeForm) && IsReady();
}
