#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

class ShineExplosion : public Artifact
{
private:
  AnimationComponent animationComponent;

public:
  ShineExplosion(Field* _field, Team _team);
  ~ShineExplosion();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
};
