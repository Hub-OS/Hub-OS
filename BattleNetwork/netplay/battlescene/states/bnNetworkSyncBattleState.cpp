#include "bnNetworkSyncBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../battlescene/States/bnCardSelectBattleState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

NetworkSyncBattleState::NetworkSyncBattleState(NetworkBattleScene* scene) :
  scene(scene),
  NetworkWaitBattleState()
{
}

NetworkSyncBattleState::~NetworkSyncBattleState()
{
}

void NetworkSyncBattleState::onStart(const BattleSceneState* next)
{
  scene->SendSyncSignal();
}
