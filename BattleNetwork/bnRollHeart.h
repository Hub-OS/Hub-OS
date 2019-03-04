#pragma once

#include "bnSpell.h"
#include "bnChipSummonHandler.h"

class RollHeart : public Spell {
public:
  RollHeart(ChipSummonHandler* _summons, int _heal);
  virtual ~RollHeart(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int heal;
  float height = 200;
  Character* caller;
  ChipSummonHandler* summons;
  bool doOnce;
}; 
