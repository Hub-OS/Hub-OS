#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "../bnArtifact.h"
#include "dynamic_object.h"
#include "../bnAnimationComponent.h"

class ScriptedArtifact : public Artifact, public dynamic_object
{
	AnimationComponent* animationComponent;
public:
	ScriptedArtifact();
	~ScriptedArtifact();

	// Centers the animation on the tile, then invokes the function assigned to onUpdate if present.
	// @param _elapsed: The amount of elapsed time since the last frame.
	void OnUpdate(double _elapsed);

	void OnDelete();
	
	// Registers an animation file with this object.
	// @param filePath: A relative file path, between the executable and the .animation file.
	// This loads a list of animations for this object to play, defaulting to an animation named "DEFAULT".
	// Also assigns a callback to make the effect delete itself upon the animation being completed.
	void SetPath(const std::string& filePath);

	// Assigns an animation to play from the list of named animations within the .animation file registered with this object.
	// @param animName: The name of the animation, within the .animation file.
	// Also assigns a callback to make the effect delete itself upon the animation being completed.
	void SetAnimation(const std::string& animName);

	// Causes the visuals of the effect to flip horizontally.
	// Used to mirror graphics between sides.
	void Flip();

	std::function<void(ScriptedArtifact&, double)> onUpdate;
};

#endif