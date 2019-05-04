#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

/**
 * @class ElementalDamage
 * @author mav
 * @date 04/05/19
 * @file bnElementalDamage.h
 * @brief <!> symbol that appears on field when elemental damage occurs
 */
class ElementalDamage : public Artifact
{
private:
  AnimationComponent animationComponent;
  float progress;

public:
  ElementalDamage(Field* field, Team team);
  ~ElementalDamage();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};
