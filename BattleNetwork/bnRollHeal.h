#pragma once

#include "bnSpell.h"

class ChipSummonHandler;

class RollHeal : public Spell {
public:
  RollHeal(ChipSummonHandler* _summons, int heal);
  virtual ~RollHeal(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual vector<Drawable*> GetMiscComponents();

private:
  int heal;
  int random;
  ChipSummonHandler* summons;
};