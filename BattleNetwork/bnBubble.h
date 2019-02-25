#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"

class Bubble : public Obstacle {
protected:
  Animation animation;
  double speed;
  bool hit;
public:
  Bubble(Field* _field, Team _team, double speed = 1.0);
  virtual ~Bubble(void);
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
  virtual void Attack(Character* _entity);
  virtual const bool Hit(int damage, Hit::Properties props);
};