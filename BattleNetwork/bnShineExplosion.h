
#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

class ShineExplosion : public Artifact
{
private:
  AnimationComponent* animationComponent;

public:
  /**
   * @brief Loads the shine texture and animation. Sets the layer to 0*
   */
  ShineExplosion(Field* _field, Team _team);
  ~ShineExplosion();

  /**
   * @brief Loops animations
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;
  
  /**
  * @brief Removes shine explosion from play
  */
  void OnDelete() override;

  /**
   * @brief the shine effect does not move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
};
