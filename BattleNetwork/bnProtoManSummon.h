/*! \brief Protoman appears and attacks every enemy he can reach
 * 
 * NOTE: The card summon system is going under major refactoring and this
 * code will not be the same
 */

#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class CardSummonHandler;

class ProtoManSummon : public Spell {
public:
  
  /**
   * \brief Scans for enemies. Checks to see if protoman can
   * spawn in front of them. If so, the tile is stored
   * as targets.*/
  ProtoManSummon(CardSummonHandler* _summons);
  
  /**
   * @brief deconstructor
   */
  virtual ~ProtoManSummon();

  /**
   * @brief Updates position and animation
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Protoman teleports and doesn't use this
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Deals damage to the enemy with recoil
   * @param _entity
   * 
   * Spawns a sword slash artifact on top
   */
  virtual void Attack(Character* _entity);
  
  /**
   * @brief Attacks the next enemy when the animation ends
   * 
   * Configures the animation callback to approach the next tile
   * And attack the tile in front
   * If the animation ends and there are no more targets,
   * delete protoman
   */
  void DoAttackStep();

private:
  std::vector<Battle::Tile*> targets; /*!< List of every tile ProtoMan must visit */
  int random;
  CardSummonHandler* summons;
  AnimationComponent* animationComponent;
};
