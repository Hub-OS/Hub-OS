/*! \brief Wave attacks move from one side of the screen to the other
 * 
 * Wave will not delete on hit like most attacks but will continue to move
 * If the wave spawns with the team BLUE, it moves with Direction::LEFT
 * If the wave spawns with the team RED,  it moves with Direction::RIGHT
 */

#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"
class Wave : public Spell {
protected:
  Animation animation;
  double speed;
public:
  Wave(Field* _field, Team _team, double speed = 1.0);
  virtual ~Wave(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};