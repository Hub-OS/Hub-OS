#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

class AirShot : public Spell {
public:
  AirShot(Field* _field, Team _team, int damage);
  virtual ~AirShot(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Entity* _entity);
  virtual vector<Drawable*> GetMiscComponents();

private:
  int damage;
  FrameList animation;
  Animate animator;
}; 
