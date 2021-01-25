/*! \brief Part of roll summon. Heart appears from the top of the screen slowly descending 
 * 
 * NOTE: The card summon system is going under major refactoring and this
 * code will not be the same
 * 
 * NOTE: This should just be an artifact since it is used as effect
 */

#pragma once

#include "bnSpell.h"
#include "bnAnimationComponent.h"

class ParticleHeal;

class RollHeart : public Spell {
public:
  RollHeart(Character* user, int _heal);
  ~RollHeart();

  /**
   * @brief Descend and then heal the player
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief does not move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
  
  /**
   * @brief Does nothing
   * @param _entity
   */
  void Attack(Character* _entity) override; 

  /** 
  * @brief Does nothing/
  */
  void OnDelete() override;

private:
  int heal; /*!< How much to heal */
  float height; /*!< The start height of the heart */
  Character* user; /*!< The character that used the card */
  AnimationComponent* animationComponent;
  bool spawnFX{ false }; /*!< Flag to restore health once */
  ParticleHeal* healFX{ nullptr }; /*!< Heal animation effect*/
};
