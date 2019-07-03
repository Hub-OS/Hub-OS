/*! \brief Wave attacks move from one side of the screen to the other
 * 
 * Wave will not delete on hit like most attacks but will continue to move
 * If the wave spawns with the team BLUE, it moves with Direction::LEFT
 * If the wave spawns with the team RED,  it moves with Direction::RIGHT
 */

#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"
class Wave : public Spell {
protected:
  AnimationComponent* animation;
  double speed;
public:
  Wave(Field* _field, Team _team, double speed = 1.0);
  virtual ~Wave();

  virtual void OnUpdate(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};