#pragma once

#include "bnSpell.h"

class ChipSummonHandler;

class RollHeal : public Spell {
public:
  RollHeal(ChipSummonHandler* _summons, int heal);
  virtual ~RollHeal();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int heal;
  int random;
  ChipSummonHandler* summons;
};
