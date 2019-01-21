#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

class Cannon : public Spell {
public:
  Cannon(Field* _field, Team _team, int damage);
  virtual ~Cannon(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Entity* _entity);
  virtual vector<Drawable*> GetMiscComponents();

private:
  int damage;
  float cooldown;
  FrameList animation;
  Animate animator;
};