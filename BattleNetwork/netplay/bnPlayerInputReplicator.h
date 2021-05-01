#pragma once
#include <assert.h>
#include "../bnComponent.h"
#include "battlescene/bnNetworkBattleScene.h"

class PlayerInputReplicator final : public Component {
  NetworkBattleScene* nbs{ nullptr };

public:
  PlayerInputReplicator(Entity* owner) : Component(owner, Component::lifetimes::battlestep) {

  }

  ~PlayerInputReplicator() {
  }

  void OnUpdate(double) override { }

  NetPlayFlags& GetNetPlayFlags() { 
    assert(nbs && "Component must be injected into NetworkBattleScene");
    return nbs->remoteState; 
  }

  void SendShootSignal() {
    assert(nbs && "Component must be injected into NetworkBattleScene");

    nbs->sendShootSignal();
  }

  void SendChargeSignal(const bool state) {
    assert(nbs && "Component must be injected into NetworkBattleScene");

    nbs->sendChargeSignal(state);
  }

  void SendUseSpecialSignal() {
    assert(nbs && "Component must be injected into NetworkBattleScene");

    nbs->sendUseSpecialSignal();
  }

  void SendTileSignal(const int x, const int y) {
    assert(nbs && "Component must be injected into NetworkBattleScene");

    nbs->sendTileCoordSignal(x, y);
  }

  void Inject(BattleSceneBase& bs) {
    auto* networkedbs = dynamic_cast<NetworkBattleScene*>(&bs);
    if (networkedbs) {
      nbs = networkedbs;
    }
  }
};