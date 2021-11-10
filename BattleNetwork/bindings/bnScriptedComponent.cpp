#include "bnScriptedComponent.h"
#include "../bnCharacter.h"
#include "../bnSolHelpers.h"
#include "bnWeakWrapper.h"

ScriptedComponent::ScriptedComponent(std::weak_ptr<Entity> owner, Component::lifetimes lifetime) :
    Component(owner, lifetime)
{
}

ScriptedComponent::~ScriptedComponent()
{
}

void ScriptedComponent::Init() {
  weakWrap = WeakWrapper(weak_from_base<ScriptedComponent>());
}

void ScriptedComponent::OnUpdate(double dt)
{
  if (update_func.valid()) 
  {
    auto result = CallLuaCallback(update_func, weakWrap, dt);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedComponent::Inject(BattleSceneBase& scene)
{
  if (scene_inject_func.valid()) 
  {
    auto result = CallLuaCallback(scene_inject_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  // the component is now injected into the scene's update loop
  // because the character's update loop is only called when they are on the field
  // this way the timer can keep ticking

  scene.Inject(shared_from_this());
}
