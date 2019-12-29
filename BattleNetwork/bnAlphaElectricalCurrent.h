#pragma once
#include "bnSpell.h"
// Should only used in Alpha's Electric attack state
// Covers top and bottom rows with a shared hitbox (counts as 1 hit)
// And toggles to the middle row
// This counts as one reptition
// The hitboxes are dropped one the frames when the electricity is visible
// This class lets Alpha's state know when 7 repititions are over

class AnimationComponent;

class AlphaElectricCurrent : public Spell {
  AnimationComponent* anim;
  int count;
  int countMax;
public:
  AlphaElectricCurrent(Field* field, Team team, int count);
  ~AlphaElectricCurrent();

  void OnSpawn(Battle::Tile& start);

  // Inherited via Spell
  void OnUpdate(float _elapsed) override;
  void Attack(Character * _entity) override;
};