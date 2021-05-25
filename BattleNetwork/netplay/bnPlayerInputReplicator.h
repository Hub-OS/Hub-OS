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

  void OnUpdate(double) override { }

  NetPlayFlags& GetNetPlayFlags() { 
    assert(networkbs && "Component must be injected into NetworkBattleScene");
    return networkbs->remoteState;
  }

  void SendShootSignal() {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendShootSignal();
  }

  void SendChargeSignal(const bool state) {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendChargeSignal(state);
  }

  void SendUseSpecialSignal() {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendUseSpecialSignal();
  }

  void SendTileSignal(const int x, const int y) {
    assert(networkbs && "Component must be injected into NetworkBattleScene");

    networkbs->sendTileCoordSignal(x, y);
  }

  void Inject(BattleSceneBase& bs) {
    networkbs = dynamic_cast<NetworkBattleScene*>(&bs);
  }
};