#pragma once
#include <assert.h>
#include "../bnComponent.h"
#include "battlescene/bnNetworkBattleScene.h"

class PlayerInputReplicator final : public Component {
  NetworkBattleScene* networkbs{ nullptr };

public:
  PlayerInputReplicator(Entity* owner) : Component(owner, Component::lifetimes::battlestep) {

  }

  ~PlayerInputReplicator() { }

  void OnUpdate(double) override {}

  const double GetAvgLatency() const {
    assert(networkbs && "Component must be injected into NetworkBattleScene");
    return networkbs->GetAvgLatency();
  }

  NetPlayFlags& GetNetPlayFlags() { 
    assert(networkbs && "Component must be injected into NetworkBattleScene");
    return networkbs->remoteState;
  }

  void SendInputEvents(const std::vector<InputEvent>& events) {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendInputEvents(events);
  }

  void SendTileSignal(const int x, const int y) {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendTileCoordSignal(x, y);
  }

  void Inject(BattleSceneBase& bs) {
    networkbs = dynamic_cast<NetworkBattleScene*>(&bs);
  }
};