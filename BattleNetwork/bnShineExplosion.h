
#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

class ShineExplosion : public Artifact
{
private:
  std::shared_ptr<AnimationComponent> animationComponent;

public:
  /**
   * @brief Loads the shine texture and animation. Sets the layer to 0*
   */
  ShineExplosion();
  ~ShineExplosion();

  /**
   * @brief Loops animations
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
  * @brief Removes shine explosion from play
  */
  void OnDelete() override;
};
