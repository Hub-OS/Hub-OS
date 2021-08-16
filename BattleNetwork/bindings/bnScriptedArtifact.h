#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "../bnArtifact.h"
#include "dynamic_object.h"
#include "../bnAnimationComponent.h"

	/**
	 * \class ScriptedArtifact
	 * \brief An object in control of battlefield visual effects, with support for Lua scripting.
	 * 
	 */
class ScriptedArtifact final : public Artifact, public dynamic_object
{
	AnimationComponent* animationComponent{ nullptr };
	sf::Vector2f scriptedOffset{ };
public:
	ScriptedArtifact();
	~ScriptedArtifact();

	/**
	 * Centers the animation on the tile, offsets it by its internal offsets, then invokes the function assigned to onUpdate if present.
	 * @param _elapsed: The amount of elapsed time since the last frame.
	 */
	void OnUpdate(double _elapsed);

	/**
	 *
	 */
	void OnDelete();

	/**
	 * \brief Causes the visuals of the effect to flip horizontally.
	 * Most often used to mirror graphics between sides.
	 */
	void Flip();

	void SetAnimation(const std::string& path);
	Animation& GetAnimationObject();
	Battle::Tile* GetCurrentTile() const;

	/**
	 * \brief Callback function that, when registered, is called on every frame.
	 * Specific names are up to the user, recommend "on_update" or some variation for parity with other Lua scripts to avoid confusion.
	 */
	std::function<void(ScriptedArtifact&, double)> onUpdate;
};

#endif