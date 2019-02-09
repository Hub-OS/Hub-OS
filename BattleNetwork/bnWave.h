#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"
class Wave : public Spell {
protected:
  Animation animation;
  double speed;
public:
  Wave(Field* _field, Team _team, double speed = 1.0);
  virtual ~Wave(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual vector<Drawable*> GetMiscComponents();
};