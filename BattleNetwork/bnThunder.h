
/*! \brief Thunder crawls slowly in the field until it hits the nearest enemy
 * 
 * Thunder will apply stun damage on the enemy it hits. Every update loop,
 * it queries the closest enemy on the field and moves in that direction
 * towards the target.
 */
#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Thunder : public Spell {
protected:
  Animation animation;
  double elapsed;
  sf::Time timeout; /**< When time is up, thunder is removed from play */
  Entity* target; /**< The current enemy to approach */
  EntityRemoveCallback* targetRemoveCallback{ nullptr };

public:
  Thunder(Team _team);
  ~Thunder();
  
  /**
   * @brief Thunder can always move to a tile no matter the conditions
   * @param tile
   * @return Always returns true
   */
  bool CanMoveTo(Battle::Tile* tile) override;
  
  /**
   * @brief If target is null, query the field for all character enemies and track the closest one
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Attacks enemy entities. If succesful, thunder is deleted.
   * @param _entity character to attack
   */
  void Attack(Character* _entity) override;

  /** 
  * @brief Does nothing
  */
  void OnDelete() override;
}; 
