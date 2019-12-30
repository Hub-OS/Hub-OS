#pragma once
#include "bnSpell.h"
#include "bnAnimator.h"

/**
 * @class AirShot
 * @author mav
 * @date 13/05/19
 * @brief Attack that pushes opponent back one tile
 */
class AirShot : public Spell {
public:
  AirShot(Field* _field, Team _team, int damage);
  ~AirShot();
  void OnUpdate(float _elapsed);
  void Attack(Character* _entity);
  bool CanMoveTo(Battle::Tile* next);
private:
  int damage;
  float cooldown, progress, hitHeight;
  int random;
  bool hit;
};
