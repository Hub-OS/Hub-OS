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
  virtual ~Cannon();

  virtual void OnUpdate(float _elapsed);
  virtual bool CanMoveTo(Battle::Tile* next);
  virtual void Attack(Character* _entity);

private:
  int damage;
  int random;
  float cooldown, progress, hitHeight;
  bool hit;
};