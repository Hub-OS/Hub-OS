#pragma once

#include "bnSpell.h"
#include "bnChipSummonHandler.h"

class Player;

class RollHeart : public Spell {
public:
  RollHeart(ChipSummonHandler* _summons, Player* _player, int _heal);
  virtual ~RollHeart(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual vector<Drawable*> GetMiscComponents();

private:
  int heal;
  float height = 200;
  Player* player;
  ChipSummonHandler* summons;
  bool doOnce;
}; 
