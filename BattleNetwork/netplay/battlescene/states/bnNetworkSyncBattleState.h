#pragma once

#include "../../battlescene/bnNetworkBattleSceneState.h"

struct BattleSceneState;
class Player;

/*
    \brief This state will resync with the remote player before starting the next state
*/
struct NetworkSyncBattleState final : public NetworkBattleSceneState {
  bool firstConnection{ true }; //!< We need to do some extra setup for players if this is their first connection
  bool adjustedForFormState{ false }; //!< Denotes whether we have forced a resync coming out of form state
  Player *&remotePlayer;

  NetworkSyncBattleState(Player*& remotePlayer);
  ~NetworkSyncBattleState();

  void onStart(const BattleSceneState* next) override;
  void onEnd(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsSyncronized();
};