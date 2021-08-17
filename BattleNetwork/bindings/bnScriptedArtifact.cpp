#ifdef BN_MOD_SUPPORT
#include "bnScriptedArtifact.h"
#include "../bnTile.h"

ScriptedArtifact::ScriptedArtifact() :
	Artifact()
{
	animationComponent = new AnimationComponent(this);
	RegisterComponent(animationComponent);

	setScale(2.f, 2.f);
}

ScriptedArtifact::~ScriptedArtifact() { }

void ScriptedArtifact::OnUpdate(double _elapsed)
{
	if (update_func)
		update_func(*this, _elapsed);
}

void ScriptedArtifact::OnDelete()
{
	Remove();
}

void ScriptedArtifact::Flip()
{
	auto scale = getScale();

	setScale( scale.x * -1, scale.y );
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