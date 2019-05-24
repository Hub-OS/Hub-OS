#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

/**
 * @class AirShot
 * @author mav
 * @date 13/05/19
 * @brief Attack that pushes opponent back one tile
 */
class AirShot : public Spell {
public:
  AirShot(Field* _field, Team _team, int damage);
  virtual ~AirShot();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
private:
  int damage;
  float cooldown, progress, hitHeight;
  int random;
  bool hit;
}; 
