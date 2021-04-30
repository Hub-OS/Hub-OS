#pragma once

#include "../../battlescene/bnNetworkBattleSceneState.h"

struct BattleSceneState;
class Player;

/*
    \brief This state will connect to the remote player before starting battle
*/
struct ConnectRemoteBattleState final : public NetworkBattleSceneState {
  Player *&remotePlayer;

  ConnectRemoteBattleState(Player*& remotePlayer);
  ~ConnectRemoteBattleState();

  void onStart(const BattleSceneState* next) override;
  void onEnd(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;

  bool IsConnected();
};