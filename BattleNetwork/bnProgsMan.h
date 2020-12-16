/*! \brief ProgsMan is boss that throws Prog Bombs and punches the player
 * 
 * ProgsMan is a Character which means he has health and can take damage.
 * He uses a State Machine for his AI.
 * 
 * When his health is below or equal to zero, he goes into navi delete state.
 */
#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnProgsManIdleState.h"
#include "bnProgsManHitState.h"
#include "bnProgsManMoveState.h"
#include "bnProgsManPunchState.h"
#include "bnProgsManShootState.h"
#include "bnProgsManThrowState.h"
#include "bnAnimationComponent.h"
#include "bnBossPatternAI.h"

class ProgsMan : public Character, public BossPatternAI<ProgsMan> {
public:
  friend class ProgsManIdleState;
  friend class ProgsManMoveState;
  using DefaultState = ProgsManIdleState;

  /**
   * \brief Loads resources and sets health
   */
  ProgsMan(Rank _rank);
  
  /**
   * @brief deconstructor
   */
  ~ProgsMan();

  /**
   * @brief Calls Character::Update() for battle resolution and updates animation
   * @param _elapsed
   * 
   * When health is zero, changes to NaviExplosion state
   */
  void OnUpdate(double _elapsed);

  void OnDelete();

  /**
   * @brief Returns progsman's height 
   * @return const float
   */
  const float GetHeight() const;
private:
  AnimationComponent* animationComponent; /*!< component animates entities*/

};
