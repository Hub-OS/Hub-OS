#pragma once
#include "bnSpell.h"

class BasicSword : public Spell {
public:
  BasicSword(Field* _field, Team _team, int damage);
  virtual ~BasicSword(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual vector<Drawable*> GetMiscComponents();

private:
  int damage;
  float cooldown;
};
