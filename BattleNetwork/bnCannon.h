#pragma once
#include "bnSpell.h"
#include "bnAnimator.h"

/**
 * @class Cannon
 * @author mav
 * @date 05/05/19
 * @brief Legacy code. Cannon's attack.
 */
class Cannon : public Spell {
public:
  Cannon(Team _team,int damage);
  ~Cannon();

  void OnUpdate(double _elapsed) override;
  bool CanMoveTo(Battle::Tile* next) override;
  void Attack(std::shared_ptr<Character> _entity) override;
  void OnDelete() override;

private:
  int damage;
  int random;
  double cooldown, progress;
  float hitHeight;
  bool hit;
};