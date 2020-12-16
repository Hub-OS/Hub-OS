#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

/**
 * @class Elecpulse
 * @author mav
 * @date 04/05/19
 * @brief 2 tile-wide attack that stuns and pulls opponents in 1 tile
 */
class Elecpulse : public Spell {
public:
  Elecpulse(Field* _field, Team _team, int damage);
  ~Elecpulse();

  void OnSpawn(Battle::Tile& start) override;
  void OnUpdate(double _elapsed) override;
  void OnDelete() override;
  bool Move(Direction _direction) override;
  void Attack(Character* _entity) override;

private:
  int damage;
  int random;
  float cooldown, progress;
  AnimationComponent* animation;
  std::list<int> taggedCharacters; //< We share a hit box, don't attack these
};
