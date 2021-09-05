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
  std::shared_ptr<AnimationComponent> animation;
  double speed;
  bool crackTiles{ false };
  bool poisonTiles{ false };
public:
  Wave(Team _team,double speed = 1.0, int damage = 10);
  ~Wave();

  void OnUpdate(double _elapsed) override;
  void Attack(std::shared_ptr<Character> _entity) override;
  void OnDelete() override;
  void PoisonTiles(bool state);
  void CrackTiles(bool state);
};