#pragma once
#include "bnArtifact.h"
#include "bnAnimationComponent.h"

class Field;

/**
 * @class ElementalDamage
 * @author mav
 * @date 04/05/19
 * @brief <!> symbol that appears on field when elemental damage occurs
 */
class ElementalDamage : public Artifact
{
private:
  AnimationComponent animationComponent;
  double progress;

public:
  ElementalDamage();
  ~ElementalDamage();

  /**
   * @brief Grow and shrink quickly. Appear over the sprite.
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;

  bool Move(Direction _direction) override;

  void OnDelete() override;
};
