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
	 * \brief Registers an animation file with this object.
	 *
	 * @param filePath: A relative file path, between the executable and the .animation file.
	 * This loads a list of animations for this object to play, defaulting to an animation named "DEFAULT".
	 * Also assigns a callback to make the effect delete itself upon the animation being completed.
	 */
	void SetPath(const std::string& filePath);

	/**
	 * \brief Assigns an animation to play from the list of named animations within the .animation file registered with this object.
	 * @param animName: The name of the animation, within the .animation file.
	 * Also assigns a callback to make the effect delete itself upon the animation being completed.
	*/
	void SetAnimation(const std::string& animName);

	/**
	 * \brief Causes the visuals of the effect to flip horizontally.
	 * Most often used to mirror graphics between sides.
	 */
	void Flip();

	/**
	 * \brief Callback function that, when registered, is called on every frame.
	 * Specific names are up to the user, recommend "on_update" or some variation for parity with other Lua scripts to avoid confusion.
	 */
	std::function<void(ScriptedArtifact&, double)> onUpdate;

	void SetTileOffset(float x, float y);
	const sf::Vector2f& GetTileOffset() const;
};

#endif