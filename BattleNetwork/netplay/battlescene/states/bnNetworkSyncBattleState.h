#pragma once

#include "bnNetworkWaitBattleState.h"

struct BattleSceneState;
class CardSelectBattleState;
class NetworkBattleScene;

/*
    \brief This state will resync with the remote player before starting the next state
*/
struct NetworkSyncBattleState final : public NetworkWaitBattleState {
  NetworkBattleScene* scene{ nullptr };

  NetworkSyncBattleState(NetworkBattleScene* scene);
  ~NetworkSyncBattleState();
  void onStart(const BattleSceneState* next) override;
};
