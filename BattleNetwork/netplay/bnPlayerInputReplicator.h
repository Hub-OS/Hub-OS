#pragma once
#include "../bnComponent.h"
#include "bnNetworkBattleScene.h"

class PlayerInputReplicator final : public Component {
  NetworkBattleScene* nbs{ nullptr };

public:
  PlayerInputReplicator(Entity* owner) : Component(owner) {

  }

  ~PlayerInputReplicator() {
  }

  void OnUpdate(float) override { }

  NetPlayFlags& GetNetPlayFlags() { return nbs->remoteState; }

  void SendShootSignal() {
    nbs->sendShootSignal();
  }

  void SendChargeSignal() {
    nbs->sendChargeSignal();
  }

  void SendUseSpecialSignal() {
    nbs->sendUseSpecialSignal();
  }

  void SendMoveSignal(Direction dir) {
    nbs->sendMoveSignal(dir);
  }

  void Inject(BattleScene& bs) {
    auto* networkedbs = dynamic_cast<NetworkBattleScene*>(&bs);
    if (networkedbs) {
      nbs = networkedbs;
    }
  }
};