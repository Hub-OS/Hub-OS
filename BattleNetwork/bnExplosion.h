#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

class Explosion : public Artifact
{
private:
  AnimationComponent animationComponent;
  int numOfExplosions;
  sf::Vector2f offset;
  int count;
  Explosion* root;
public:
  Explosion(Field* _field, Team _team, int _numOfExplosions=1);
  Explosion(const Explosion& copy);
  ~Explosion();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
  vector<Drawable*> GetMiscComponents();

  void IncrementExplosionCount();

};

