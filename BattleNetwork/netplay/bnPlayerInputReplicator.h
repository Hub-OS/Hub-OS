#pragma once
#include "../bnComponent.h"
#include "bnNetworkBattleScene.h"

class PlayerInputReplicator final : public Component {
  NetworkBattleSceneBase* nbs{ nullptr };

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

  void SendChargeSignal(const bool state) {
    nbs->sendChargeSignal(state);
  }

  void SendUseSpecialSignal() {
    nbs->sendUseSpecialSignal();
  }

  void SendMoveSignal(Direction dir) {
    if (dir == Direction::none) return;
    nbs->sendMoveSignal(dir);
  }

  void SendTileSignal(const int x, const int y) {
    nbs->sendTileCoordSignal(x, y);
  }

  void Inject(BattleSceneBase& bs) {
    auto* networkedbs = dynamic_cast<NetworkBattleSceneBase*>(&bs);
    if (networkedbs) {
      nbs = networkedbs;
    }
  }
};