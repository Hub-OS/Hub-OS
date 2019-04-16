#pragma once
#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class GuardHit : public Artifact
{
private:
  AnimationComponent animationComponent;
  float w; float h;
  bool center;

public:
  GuardHit(Field* _field, Character* hit, bool center = false);
  ~GuardHit();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};

