#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class CrackShot : public Spell {
protected:
  AnimationComponent* animation; 
  double speed; 
public:

  CrackShot(Team _team,Battle::Tile* t);

  /**
   * @brief deconstructor
   */
  ~CrackShot();

  /**
   * @brief CrackShot flies through the air
   * @param tile
   * @return true
   */
  bool CanMoveTo(Battle::Tile* tile) override;

  /**
   * @brief Moves in one direction
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
  void Attack(Character* _entity) override;

  /**
  * @brief Does nothing */
  void OnDelete() override;
};
