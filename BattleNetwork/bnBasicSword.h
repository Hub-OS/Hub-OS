#pragma once
#include "bnSpell.h"

/**
 * @class BasicSword
 * @author mav
 * @date 13/05/19
 * @file bnBasicSword.h
 * @brief A single-tile hit box that is used for sword chips. 
 * 
 * @warnig We already have a Hitbox class now and makes this code obsolete
 */
class BasicSword : public Spell {
public:
  BasicSword(Field* _field, Team _team, int damage);
  virtual ~BasicSword(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
private:
  int damage;
  float cooldown;
};
