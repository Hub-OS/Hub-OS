#pragma once

#include "bnNetworkWaitBattleState.h"

struct BattleSceneState;
class CardSelectBattleState;
class Player;
class NetworkBattleScene;

/*
    \brief This state will resync with the remote player before starting the next state
*/
struct NetworkCardWaitBattleState final : public NetworkWaitBattleState {
  bool firstConnection{ true }; //!< We need to do some extra setup for players if this is their first connection
  std::shared_ptr<Player>& remotePlayer;
  CardSelectBattleState*& cardSelectState;
  NetworkBattleScene* scene{ nullptr };

  NetworkCardWaitBattleState(std::shared_ptr<Player>& remotePlayer, NetworkBattleScene* scene);
  ~NetworkCardWaitBattleState();
  void onStart(const BattleSceneState* next) override;
  void onEnd(const BattleSceneState* last) override;
  bool IsRemoteConnected();
  bool SelectedNewChips();
  bool NoConditions(); //!< Used when the forms and combo conditions are not met
  bool HasForm();
};