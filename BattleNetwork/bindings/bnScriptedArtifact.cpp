#ifdef BN_MOD_SUPPORT
#include "bnScriptedArtifact.h"
#include "../bnTile.h"
#include "../bnSolHelpers.h"

ScriptedArtifact::ScriptedArtifact() :
  Artifact()
{
  setScale(2.f, 2.f);
}

void ScriptedArtifact::Init()
{
  Artifact::Init();
  animationComponent = std::make_shared<AnimationComponent>(weak_from_this());
  RegisterComponent(animationComponent);
  weakWrap = WeakWrapper(weak_from_base<ScriptedArtifact>());
}

ScriptedArtifact::~ScriptedArtifact() { }

void ScriptedArtifact::OnUpdate(double _elapsed)
{
  if (update_func.valid()) 
  {
    auto result = CallLuaCallback(update_func, weakWrap, _elapsed);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedArtifact::OnSpawn(Battle::Tile& tile)
{
  if (on_spawn_func.valid()) 
  {
    auto result = CallLuaCallback(on_spawn_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedArtifact::OnDelete()
{
  if (delete_func.valid()) 
  {
    auto result = CallLuaCallback(delete_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  Erase();
}

bool ScriptedArtifact::CanMoveTo(Battle::Tile* next)
{
  if (can_move_to_func.valid()) 
  {
    auto result = CallLuaCallbackExpectingValue<bool>(can_move_to_func, next);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }

    return result.value();
  }

  return false;
}

void ScriptedArtifact::SetAnimation(const std::string& path)
{
  animationComponent->SetPath(path);
  animationComponent->Load();
}

Animation& ScriptedArtifact::GetAnimationObject()
{
  return animationComponent->GetAnimationObject();
}

Battle::Tile* ScriptedArtifact::GetCurrentTile() const
{
  return GetTile();
}

#endif