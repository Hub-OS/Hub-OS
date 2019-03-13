#pragma once

#include "bnSpell.h"

class ChipSummonHandler;

class ProtoManSummon : public Spell {
public:
  ProtoManSummon(ChipSummonHandler* _summons);
  virtual ~ProtoManSummon();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  
  void DoAttackStep();

private:
  std::vector<Battle::Tile*> targets;
  int random;
  ChipSummonHandler* summons;
};
