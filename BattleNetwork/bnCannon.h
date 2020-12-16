#pragma once
#include "bnSpell.h"
#include "bnAnimator.h"

/**
 * @class Cannon
 * @author mav
 * @date 05/05/19
 * @brief Legacy code. Cannon's attack.
 * @warning should be update code. Uses old battle system logic. 
 */
class Cannon : public Spell {
public:
  Cannon(Field* _field, Team _team, int damage);
  ~Cannon();

  void OnUpdate(double _elapsed) override;
  bool CanMoveTo(Battle::Tile* next) override;
  void Attack(Character* _entity) override;
  void OnDelete() override;

private:
  int damage;
  int random;
  double cooldown, progress;
  float hitHeight;
  bool hit;
};