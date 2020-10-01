#include "bnNetPlayFlags.h"
#include "../bnComponent.h"

class PlayerNetworkProxy final : public Component {
  NetPlayFlags& npf;

public:
  PlayerNetworkProxy(Entity* owner, NetPlayFlags& flags) : Component(owner), npf(flags) {

  }

  virtual ~PlayerNetworkProxy() {
  }

  NetPlayFlags& GetNetPlayFlags() { return npf; }

  void OnUpdate(float) override { }

  void Inject(BattleSceneBase& bs) override {
    // do nothing
  }
};