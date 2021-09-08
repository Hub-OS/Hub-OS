#include "bnScriptedComponent.h"
#include "../bnCharacter.h"

ScriptedComponent::ScriptedComponent(std::weak_ptr<Character> owner, Component::lifetimes lifetime) :
    Component(owner, lifetime)
{
}

ScriptedComponent::~ScriptedComponent()
{
}

void ScriptedComponent::OnUpdate(double dt)
{
  if (update_func) {
    update_func(shared_from_base<ScriptedComponent>(), dt);
  }
}

void ScriptedComponent::Inject(BattleSceneBase& scene)
{
  if (scene_inject_func) {
    scene_inject_func(shared_from_base<ScriptedComponent>());
  }

  // the component is now injected into the scene's update loop
// because the character's update loop is only called when they are on the field
// this way the timer can keep ticking

  scene.Inject(shared_from_this());
}

std::shared_ptr<Character> ScriptedComponent::GetOwnerAsCharacter()
{
  return this->GetOwnerAs<Character>();
}
