#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ParticlePoof : public Artifact {
private:
  Animation animation;
  sf::Sprite poof;
public:
  ParticlePoof(Field* field);
  ~ParticlePoof();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
};