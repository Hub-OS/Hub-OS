#pragma once

#include "../../battlescene/bnNetworkBattleSceneState.h"

struct BattleSceneState;
class CardSelectBattleState;
class Player;
class NetworkBattleScene;

/*
    \brief This state will resync with the remote player before starting the next state
*/
struct NetworkSyncBattleState final : public NetworkBattleSceneState {
  bool firstConnection{ true }; //!< We need to do some extra setup for players if this is their first connection
  bool synchronized{ false }; 
  frame_time_t flicker{};
  std::shared_ptr<Player>& remotePlayer;
  CardSelectBattleState*& cardSelectState;
  NetworkBattleScene* scene{ nullptr };

  NetworkSyncBattleState(std::shared_ptr<Player>& remotePlayer, NetworkBattleScene* scene);
  ~NetworkSyncBattleState();
  void Synchronize(); // mark this state for synchronization
  const bool IsSynchronized() const;
  void onStart(const BattleSceneState* next) override;
  void onEnd(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsRemoteConnected();
  bool SelectedNewChips();
  bool NoConditions(); //!< Used when the forms and combo conditions are not met
  bool HasForm();
};