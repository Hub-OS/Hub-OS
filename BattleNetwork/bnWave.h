/*! \brief Wave attacks move from one side of the screen to the other
 * 
 * Wave will not delete on hit like most attacks but will continue to move
 * If the wave spawns with the team BLUE, it moves with Direction::left
 * If the wave spawns with the team RED,  it moves with Direction::right
 */

#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"
class Wave : public Spell {
protected:
  AnimationComponent* animation;
  double speed;
  bool crackTiles{ false };
public:
  Wave(Field* _field, Team _team, double speed = 1.0);
  ~Wave();

  void OnUpdate(float _elapsed) override;
  bool Move(Direction _direction) override;
  void Attack(Character* _entity) override;
  void OnDelete() override;
  void CrackTiles(bool state);
};