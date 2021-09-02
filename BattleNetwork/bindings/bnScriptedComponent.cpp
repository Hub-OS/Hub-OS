#include "bnScriptedComponent.h"
#include "../bnCharacter.h"

ScriptedComponent::ScriptedComponent(Character* owner, Component::lifetimes lifetime) :
    Component(owner, lifetime)
{
}

ScriptedComponent::~ScriptedComponent()
{
}

void ScriptedComponent::OnUpdate(double dt)
{
  if (update_func) {
    update_func(this, dt);
  }
}

void ScriptedComponent::Inject(BattleSceneBase& scene)
{
  if (scene_inject_func) {
    scene_inject_func(this);
  }

  // the component is now injected into the scene's update loop
// because the character's update loop is only called when they are on the field
// this way the timer can keep ticking

  scene.Inject(this);
}

Character* ScriptedComponent::GetOwnerAsCharacter()
{
  return this->GetOwnerAs<Character>();
}
