#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "../bnArtifact.h"
#include "dynamic_object.h"
#include "../bnAnimationComponent.h"
#include "bnWeakWrapper.h"

	/**
	 * \class ScriptedArtifact
	 * \brief An object in control of battlefield visual effects, with support for Lua scripting.
	 * 
	 */
class ScriptedArtifact final : public Artifact, public dynamic_object
{
	std::shared_ptr<AnimationComponent> animationComponent{ nullptr };
	sf::Vector2f scriptedOffset{ };
	bool flip{true};

public:
	ScriptedArtifact();
	~ScriptedArtifact();

	void Init() override;

	/**
	 * Centers the animation on the tile, offsets it by its internal offsets, then invokes the function assigned to onUpdate if present.
	 * @param _elapsed: The amount of elapsed time since the last frame.
	 */
	void OnUpdate(double _elapsed) override;
	void OnDelete() override;
	bool CanMoveTo(Battle::Tile* next) override;
	void OnSpawn(Battle::Tile& spawn) override;
	void NeverFlip(bool enabled);

	void SetAnimation(const std::string& path);
	Animation& GetAnimationObject();
	Battle::Tile* GetCurrentTile() const;

	/**
	 * \brief Callback function that, when registered, is called on every frame.
	 */
	std::function<void(WeakWrapper<ScriptedArtifact>, double)> updateCallback;
	std::function<void(WeakWrapper<ScriptedArtifact>, Battle::Tile&)> spawnCallback;
	std::function<bool(Battle::Tile&)> canMoveToCallback;
	std::function<void(WeakWrapper<ScriptedArtifact>)> deleteCallback;
};

#endif