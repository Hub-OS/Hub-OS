#pragma once
#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class GuardHit : public Artifact
{
private:
  AnimationComponent animationComponent;
  float w; float h;

public:
  GuardHit(Field* _field, Character* hit);
  ~GuardHit();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
  vector<Drawable*> GetMiscComponents();

};

