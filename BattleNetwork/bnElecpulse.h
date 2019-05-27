#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

<<<<<<< HEAD
=======
/**
 * @class Elecpulse
 * @author mav
 * @date 04/05/19
 * @brief 2 tile-wide attack that stuns and pulls opponents in 1 tile
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class Elecpulse : public Spell {
public:
  Elecpulse(Field* _field, Team _team, int damage);
  virtual ~Elecpulse(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);

private:
  int damage;
  int random;
<<<<<<< HEAD
  float cooldown;
=======
  float cooldown, progress;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  AnimationComponent animation;
};
