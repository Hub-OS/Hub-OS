#pragma once
#include "bnObstacle.h"
#include "bnComponent.h"

// Hit boxes can attack and be attacked: obstacle traits
class SharedHitBox : public Obstacle {
public:
	
  SharedHitBox(Spell* owner, float duration);
  virtual ~SharedHitBox();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual const bool Hit(Hit::Properties props);
  
private:
  float cooldown;
  Spell* owner;
};
