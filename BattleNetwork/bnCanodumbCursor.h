#pragma once

#include "bnArtifact.h"
#include "bnField.h"

#include "bnCanodumb.h"
#include "bnCanodumbAttackState.h"

class CanodumbCursor : public Artifact
{
private:
  Canodumb* parent;
  Entity* target;
  float movecooldown;
  float maxcooldown;
  Direction direction;

  // Frame select through animation system
  AnimationComponent animationComponent;
public:
  CanodumbCursor(Field* _field, Team _team, Canodumb* _parent);
  ~CanodumbCursor();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
};


