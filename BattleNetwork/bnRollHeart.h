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
  RollHeart(std::shared_ptr<Character> user, int _heal);
  ~RollHeart();

  /**
   * @brief Descend and then heal the player
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Does nothing
   * @param _entity
   */
  void Attack(std::shared_ptr<Character> _entity) override; 

  /** 
  * @brief Does nothing/
  */
  void OnDelete() override;

private:
  int heal; /*!< How much to heal */
  float height; /*!< The start height of the heart */
  std::shared_ptr<Character> user; /*!< The character that used the card */
  std::shared_ptr<AnimationComponent> animationComponent;
  bool spawnFX{ false }; /*!< Flag to restore health once */
  std::shared_ptr<ParticleHeal> healFX{ nullptr }; /*!< Heal animation effect*/
};
