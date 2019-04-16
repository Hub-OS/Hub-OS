#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class MetalBlade : public Spell {
protected:
  Animation animation;
  double speed;
public:
  MetalBlade(Field* _field, Team _team, double speed = 1.0);
  virtual ~MetalBlade(void);
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
  virtual void Attack(Character* _entity);
};