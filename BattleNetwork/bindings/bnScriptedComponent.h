#pragma once
#ifdef BN_MOD_SUPPORT

#include <functional>
#include "dynamic_object.h"
#include "../bnComponent.h"
#include "../battlescene/bnBattleSceneBase.h"

/**
 * @class ScriptedComponent
*/

class Character;

class ScriptedComponent final : public Component, public dynamic_object {
public:
  ScriptedComponent(Character* owner, Component::lifetimes lifetime);
  ~ScriptedComponent();

  void OnUpdate(double dt) override;
  void Inject(BattleSceneBase&) override;
  Character* GetOwnerAsCharacter();

  std::function<void(ScriptedComponent*, double)> update_func;
  std::function<void(ScriptedComponent*)> scene_inject_func;
};

#endif