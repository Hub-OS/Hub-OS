#pragma once
#include "bnSpell.h"

/**
 * @class BasicSword
 * @author mav
 * @date 13/05/19
 * @brief A single-tile hit box that is used for sword cards. 
 * 
 * @warnig We already have a Hitbox class now and makes this code obsolete
 */
class BasicSword : public Spell {
public:
  BasicSword(Field* _field, Team _team, int damage);
  virtual ~BasicSword();

  virtual void OnUpdate(float _elapsed) override;
  virtual bool Move(Direction _direction) override;
  virtual void Attack(Character* _entity) override;
  virtual void OnDelete() override;
private:
  int damage;
  float cooldown, hitHeight;
  bool hit;
};
