#ifdef BN_MOD_SUPPORT
#include "bnScriptedArtifact.h"
#include "../bnTile.h"

ScriptedArtifact::ScriptedArtifact() :
	Artifact()
{
	animationComponent = std::make_shared<AnimationComponent>(weak_from_this());
	RegisterComponent(animationComponent);

	setScale(2.f, 2.f);
}

ScriptedArtifact::~ScriptedArtifact() { }

void ScriptedArtifact::OnUpdate(double _elapsed)
{
	ScriptedArtifact& sa = *this;
	updateCallback ? updateCallback(sa, _elapsed) : (void)0;
}

void ScriptedArtifact::NeverFlip(bool enabled)
{
	flip = !enabled;
}

void ScriptedArtifact::OnSpawn(Battle::Tile& tile)
{
	ScriptedArtifact& sa = *this;
	spawnCallback ? spawnCallback(sa, tile) : (void)0;

	if (GetTeam() == Team::blue && flip) {
		setScale(-2.f, 2.f);
	}
}

void ScriptedArtifact::OnDelete()
{
	ScriptedArtifact& sa = *this;
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