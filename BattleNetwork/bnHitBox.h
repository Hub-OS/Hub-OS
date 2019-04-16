#pragma once
#include "bnSpell.h"

class HitBox : public Spell {
public:
  HitBox(Field* _field, Team _team, int damage);
  virtual ~HitBox(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
private:
  int damage;
  float cooldown;
};
