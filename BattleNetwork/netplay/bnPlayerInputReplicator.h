#pragma once
#include <assert.h>
#include "../bnComponent.h"
#include "battlescene/bnNetworkBattleScene.h"

class PlayerInputReplicator final : public Component {
  NetworkBattleScene* networkbs{ nullptr };

public:
  PlayerInputReplicator(std::weak_ptr<Entity> owner) : Component(owner, Component::lifetimes::battlestep) {

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

    networkbs->QueueInputEvents(events);
  }

  void Inject(BattleSceneBase& bs) {
    networkbs = dynamic_cast<NetworkBattleScene*>(&bs);
  }
};