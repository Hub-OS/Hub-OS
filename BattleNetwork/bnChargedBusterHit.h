#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class ChargedBusterHit : public Artifact
{
private:
  AnimationComponent animationComponent;

public:
  ChargedBusterHit(Field* _field, Character* hit);
  ~ChargedBusterHit();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};

