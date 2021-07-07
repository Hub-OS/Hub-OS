#ifdef BN_MOD_SUPPORT
#include "bnScriptedArtifact.h"
#include "../bnTile.h"

ScriptedArtifact::ScriptedArtifact() :
	Artifact()
{
	animationComponent = new AnimationComponent(this);
	RegisterComponent(animationComponent);

	setScale(2.f, 2.f);

	auto onEnd = [this]() {
		Delete();
	};
	animationComponent->SetAnimation("DEFAULT", onEnd);
}

ScriptedArtifact::~ScriptedArtifact() { }

void ScriptedArtifact::OnUpdate(double _elapsed)
{
	setPosition( GetTile()->getPosition() + tileOffset + scriptedOffset );

	if (onUpdate)
		onUpdate(*this, _elapsed);
}

void ScriptedArtifact::OnDelete()
{
	Remove();
}

void ScriptedArtifact::SetPath(const std::string& filePath)
{
	animationComponent->SetPath(filePath);
	animationComponent->Reload();

	auto onEnd = [this]() { Delete(); };

	animationComponent->SetAnimation("DEFAULT", onEnd);
	animationComponent->SetFrame(0);
}

void ScriptedArtifact::SetAnimation(const std::string& animName)
{
	auto onEnd = [this]() {
		Delete();
	};

	animationComponent->SetAnimation(animName, onEnd);
	animationComponent->SetFrame(0);
}

void ScriptedArtifact::Flip()
{
	auto scale = getScale();

	setScale( scale.x * -1, scale.y );
}

void ScriptedArtifact::SetTileOffset(float x, float y)
{
	scriptedOffset = { x, y };
}
const sf::Vector2f& ScriptedArtifact::GetTileOffset() const
{
	return ScriptedArtifact::scriptedOffset;
}

#endif