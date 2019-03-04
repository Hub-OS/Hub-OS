#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class Elecpulse : public Spell {
public:
  Elecpulse(Field* _field, Team _team, int damage);
  virtual ~Elecpulse(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int damage;
  int random;
  float cooldown;
  AnimationComponent animation;
};
