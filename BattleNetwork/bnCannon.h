#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

/**
 * @class Cannon
 * @author mav
 * @date 05/05/19
 * @file bnCannon.h
 * @brief Legacy code. Cannon's attack.
 * @warning should be update code. Uses old battle system logic. 
 */
class Cannon : public Spell {
public:
  Cannon(Field* _field, Team _team, int damage);
  virtual ~Cannon();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int damage;
  int random;
  float cooldown;
  FrameList animation;
  Animate animator;
};