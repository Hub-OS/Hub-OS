#include "bnNetworkSyncBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../bnPlayerNetworkProxy.h"
#include "../../bnPlayerNetworkState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

NetworkSyncBattleState::NetworkSyncBattleState(Player*& remotePlayer, Handshake* handshake) :
  handshake(handshake),
  remotePlayer(remotePlayer),
  NetworkBattleSceneState()
{
}

NetworkSyncBattleState::~NetworkSyncBattleState()
{
}

void NetworkSyncBattleState::onStart(const BattleSceneState* next)
{
}

void NetworkSyncBattleState::onEnd(const BattleSceneState* last)
{
  if (firstConnection) {
    GetScene().GetPlayer()->ChangeState<PlayerControlledState>();
    auto remoteProxy = remotePlayer->GetFirstComponent<PlayerNetworkProxy>();
    remotePlayer->ChangeState<PlayerNetworkState>(remoteProxy->GetNetPlayFlags());
    firstConnection = false;
  }
}

void NetworkSyncBattleState::onUpdate(double elapsed)
{
  if (IsSyncronizedWithRemote()) {
    handshake->resync = false;
  }
}

void NetworkSyncBattleState::onDraw(sf::RenderTexture& surface)
{
}

void NetworkSyncBattleState::RemoteRequestState(Handshake::Type incoming)
{
  remoteHandshakeRequest = incoming;

  if (incoming == handshake->type) {
    handshake->established = true;
  }
  else {
    // switch into the form change handshake state if the remote is and we are ready
    if (incoming == Handshake::Type::form_change && handshake->type == Handshake::Type::battle) {
      handshake->type = Handshake::Type::form_change;
    }

    handshake->established = false;
    handshake->isClientReady = handshake->isRemoteReady = false;
    handshake->resync = true;
  }
}

bool NetworkSyncBattleState::IsSyncronized(Handshake::Type desired)
{
  bool matching_request = handshake->type == desired;
  bool syncronized = handshake->established && matching_request;
  bool ready = handshake->isClientReady && handshake->isRemoteReady;
  return syncronized && ready;
}

bool NetworkSyncBattleState::IsSyncronizedWithRemote()
{
  return IsSyncronized(this->remoteHandshakeRequest);
}
