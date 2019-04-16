#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"

class Fishy : public Obstacle {
protected:
  sf::Sprite fishy;
  double speed;
  bool hit;
public:
  Fishy(Field* _field, Team _team, double speed = 1.0);
  virtual ~Fishy(void);
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
  virtual void Attack(Character* _entity);
  virtual const bool Hit(Hit::Properties props);
};
