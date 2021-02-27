/*! \brief Protoman appears and attacks every enemy he can reach
 * 
 * NOTE: The card summon system is going under major refactoring and this
 * code will not be the same
 */

#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class Character;

class ProtoManSummon : public Spell {
public:
  
  /**
   * \brief Scans for enemies. Checks to see if protoman can
   * spawn in front of them. If so, the tile is stored
   * as targets.*/
  ProtoManSummon(Character* user, int damage);
  
  /**
   * @brief deconstructor
   */
  ~ProtoManSummon();

  /**
   * @brief Updates position and animation
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Protoman teleports and doesn't use this
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
  
  /**
   * @brief Deals damage to the enemy with recoil
   * @param _entity
   * 
   * Spawns a sword slash artifact on top
   */
  void Attack(Character* _entity) override;
  
  /**
   * @brief Attacks the next enemy when the animation ends
   * 
   * Configures the animation callback to approach the next tile
   * And attack the tile in front
   * If the animation ends and there are no more targets,
   * delete protoman
   */
  void DoAttackStep();

  /**
  * @brief Does nothing
  */
  void OnDelete() override;

  void OnSpawn(Battle::Tile& start) override;

private:
  std::vector<Battle::Tile*> targets; /*!< List of every tile ProtoMan must visit */
  int random;
  Character* user{ nullptr };
  AnimationComponent* animationComponent;
};
