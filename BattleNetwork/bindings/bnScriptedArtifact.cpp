#ifdef BN_MOD_SUPPORT
#include "bnScriptedArtifact.h"
#include "../bnTile.h"

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
}

ScriptedArtifact::~ScriptedArtifact() { }

void ScriptedArtifact::OnUpdate(double _elapsed)
{
  if (updateCallback) {
    auto artifact = WeakWrapper(weak_from_base<ScriptedArtifact>());

    try {
      updateCallback(artifact, _elapsed);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedArtifact::NeverFlip(bool enabled)
{
  flip = !enabled;
}

void ScriptedArtifact::OnSpawn(Battle::Tile& tile)
{
  if (spawnCallback) {
    auto artifact = WeakWrapper(weak_from_base<ScriptedArtifact>());

    try {
      spawnCallback(artifact, tile);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  if (GetTeam() == Team::blue && flip) {
    setScale(-2.f, 2.f);
  }
}

void ScriptedArtifact::OnDelete()
{
  if (deleteCallback) {
    auto artifact = WeakWrapper(weak_from_base<ScriptedArtifact>());

    try {
      deleteCallback(artifact);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  Remove();
}

bool ScriptedArtifact::CanMoveTo(Battle::Tile* next)
{
  if (canMoveToCallback) {
    try {
      return canMoveToCallback(*next);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
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