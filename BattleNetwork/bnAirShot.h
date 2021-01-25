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
  AirShot(Team _team,int damage);
  ~AirShot();
  void OnUpdate(double _elapsed) override;
  void Attack(Character* _entity) override;
  bool CanMoveTo(Battle::Tile* next) override;
  void OnDelete() override;
private:
  int damage;
  double cooldown, progress;
  float hitHeight;
  int random;
  bool hit;
};
