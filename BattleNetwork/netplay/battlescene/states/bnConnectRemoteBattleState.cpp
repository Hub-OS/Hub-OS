#include "bnConnectRemoteBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../bnPlayerNetworkProxy.h"
#include "../../bnPlayerNetworkState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

ConnectRemoteBattleState::ConnectRemoteBattleState(Player*& remotePlayer) : 
  remotePlayer(remotePlayer),
  NetworkBattleSceneState()
{
}

ConnectRemoteBattleState::~ConnectRemoteBattleState()
{
}

void ConnectRemoteBattleState::onStart(const BattleSceneState* next)
{
}

void ConnectRemoteBattleState::onEnd(const BattleSceneState* last)
{
  GetScene().GetPlayer()->ChangeState<PlayerControlledState>();

  auto remoteProxy = remotePlayer->GetFirstComponent<PlayerNetworkProxy>();
  remotePlayer->ChangeState<PlayerNetworkState>(remoteProxy->GetNetPlayFlags());
}

void ConnectRemoteBattleState::onUpdate(double elapsed)
{
}

void ConnectRemoteBattleState::onDraw(sf::RenderTexture& surface)
{
}

bool ConnectRemoteBattleState::IsConnected()
{
  return GetScene().GetRemoteStateFlags().isRemoteConnected;
}
