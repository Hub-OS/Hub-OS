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
  virtual ~Elecpulse();

  virtual void OnUpdate(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int damage;
  int random;
  float cooldown, progress;
  AnimationComponent* animation;
};
