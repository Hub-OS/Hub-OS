#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

/**
 * @class Cannon
 * @author mav
 * @date 05/05/19
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
<<<<<<< HEAD
  float cooldown;
=======
  float cooldown, progress, hitHeight;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  FrameList animation;
  Animate animator;
  bool hit;
  sf::Texture* texture;
};