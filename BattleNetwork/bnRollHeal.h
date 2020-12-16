/*! \brief Roll appears and attacks a random enemy 3 times 
 * 
 *  NOTE: The card summon system is going under major refactoring and this
 *  code will not be the same
 */

#pragma once

#include "bnSpell.h"
#include "bnAnimationComponent.h"

class RollHeal : public Spell {
public:

  /**
   * @brief Adds itself to the field facing the opposing team based on who summoned it
   * 
   * Prepares animations callbacks
   * @param heal how much to heal the player with
   */
  RollHeal(Field* field, Team team, Character* user, int heal);
  
  /**
   * @brief Deconstructor
   */
  ~RollHeal();

  /**
   * @brief Updates the animation
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Does not move
   * @param _direction ignored 
   * @return false
   */
  bool Move(Direction _direction) override;

  /**
   * @brief Deals damage to enemy
   * @param _entity
   */
  void Attack(Character* _entity) override;

  void OnDelete() override;

  void DropHitbox(Battle::Tile* target);

private:
  int heal;
  int random;
  AnimationComponent* animationComponent;
  Character* user{ nullptr };
};
