#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

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
