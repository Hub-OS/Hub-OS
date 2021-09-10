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
  auto sa = shared_from_base<ScriptedArtifact>();
  updateCallback ? updateCallback(sa, _elapsed) : (void)0;
}

void ScriptedArtifact::NeverFlip(bool enabled)
{
  flip = !enabled;
}

void ScriptedArtifact::OnSpawn(Battle::Tile& tile)
{
  auto sa = shared_from_base<ScriptedArtifact>();
  spawnCallback ? spawnCallback(sa, tile) : (void)0;

  if (GetTeam() == Team::blue && flip) {
    setScale(-2.f, 2.f);
  }
}

void ScriptedArtifact::OnDelete()
{
  auto sa = shared_from_base<ScriptedArtifact>();
  deleteCallback ? deleteCallback(sa) : (void)0;
  Remove();
}

bool ScriptedArtifact::CanMoveTo(Battle::Tile* next)
{
  return canMoveToCallback ? canMoveToCallback(*next) : false;
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