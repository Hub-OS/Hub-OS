#pragma once
#ifdef BN_MOD_SUPPORT

#include <functional>
#include "dynamic_object.h"
#include "../bnComponent.h"
#include "../battlescene/bnBattleSceneBase.h"
#include "bnWeakWrapper.h"

/**
 * @class ScriptedComponent
*/

class Character;

class ScriptedComponent final :
  public Component,
  public dynamic_object
{
public:
  ScriptedComponent(std::weak_ptr<Character> owner, Component::lifetimes lifetime);
  ~ScriptedComponent();

  void Init();
  void OnUpdate(double dt) override;
  void Inject(BattleSceneBase&) override;
  std::shared_ptr<Character> GetOwnerAsCharacter();

  sol::object update_func;
  sol::object scene_inject_func;
private:
  WeakWrapper<ScriptedComponent> weakWrap;
};

#endif