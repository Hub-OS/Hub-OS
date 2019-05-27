#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

<<<<<<< HEAD
=======
/**
 * @class ElementalDamage
 * @author mav
 * @date 04/05/19
 * @brief <!> symbol that appears on field when elemental damage occurs
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class ElementalDamage : public Artifact
{
private:
  AnimationComponent animationComponent;
  float progress;

public:
  ElementalDamage(Field* field, Team team);
  ~ElementalDamage();

<<<<<<< HEAD
=======
  /**
   * @brief Grow and shrink quickly. Appear over the sprite.
   * @param _elapsed
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};
