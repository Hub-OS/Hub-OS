#include "bnNetworkSyncBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../bnPlayerNetworkProxy.h"
#include "../../bnPlayerNetworkState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

NetworkSyncBattleState::NetworkSyncBattleState(Player*& remotePlayer) :
  remotePlayer(remotePlayer),
  NetworkBattleSceneState()
{
}

NetworkSyncBattleState::~NetworkSyncBattleState()
{
}

void NetworkSyncBattleState::onStart(const BattleSceneState* next)
{
  adjustedForFormState = false;
}

void NetworkSyncBattleState::onEnd(const BattleSceneState* last)
{
  if (firstConnection) {
    GetScene().GetPlayer()->ChangeState<PlayerControlledState>();
    
    if (remotePlayer) {
      if (auto remoteProxy = remotePlayer->GetFirstComponent<PlayerNetworkProxy>()) {
        remotePlayer->ChangeState<PlayerNetworkState>(remoteProxy->GetNetPlayFlags());
      }
    }

    firstConnection = false;
  }
}

void NetworkSyncBattleState::onUpdate(double elapsed)
{
}

void NetworkSyncBattleState::onDraw(sf::RenderTexture& surface)
{
}

bool NetworkSyncBattleState::IsSyncronized()
{
  // TODO: `return does_have_remote_ack;`
  return false;
}
