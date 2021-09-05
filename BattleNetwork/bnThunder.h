
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
  std::shared_ptr<Entity> target{ nullptr }; /**< The current enemy to approach */

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
   * @brief Collides with entities. Thunder is deleted.
   * @param what was collided with
   */
  void OnCollision(const std::shared_ptr<Character> _entity) override;

  /** 
  * @brief Does nothing
  */
  void OnDelete() override;

  /**
  * @brief Attacks entity
  * @param what's getting attacked
  */
  void Attack(std::shared_ptr<Character> _entity) override;
}; 
