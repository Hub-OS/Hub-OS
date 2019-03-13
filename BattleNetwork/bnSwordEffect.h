#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class SwordEffect : public Artifact {
private:
  Animation animation;
public:
  SwordEffect(Field* field);
  ~SwordEffect();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
};
