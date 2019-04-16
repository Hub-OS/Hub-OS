#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class RowHit : public Spell {
protected:
  Animation animation;
  int damage;
public:
  RowHit(Field* _field, Team _team,int damage);
  virtual ~RowHit(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};
