#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class CanonSmoke : public Artifact
{
private:
  AnimationComponent animationComponent;

public:
  CanonSmoke(Field* _field, Team _team);
  ~CanonSmoke();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
};

