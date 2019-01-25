#pragma once
#include "bnBattleScene.h"
#include "bnComponent.h"

#include <functional>
// T must be type of Entity
template<typename T>
class BattleOverTrigger : public Component {
private:
  BattleScene* scene;
  std::function<void(BattleScene&, T&)> callback;

public:
  BattleOverTrigger(T* owner, std::function<void(BattleScene&, T&)> callback) : Component(owner) {
    this->callback = callback;
  }

  virtual ~BattleOverTrigger() {

  }

  void Update(float _elapsed) {
    if (callback && this->GetOwner()) {
      if (this->scene->IsCleared()) {
        callback(*this->scene, *(dynamic_cast<T*>(this->GetOwner())));
        callback = nullptr;
      }
    }
  }

  void Inject(BattleScene& scene) {
    this->scene = &scene;
  }
};