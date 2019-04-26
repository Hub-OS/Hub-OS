/*! \brief Attack state for Starfish AI
 * 
 * Spawns `maxBubbleCount` bubbles and then goes back to
 * StarfishIdleState
 */

#pragma once
#include "bnAIState.h"
#include "bnStarfish.h"

class StarfishAttackState : public AIState<Starfish>
{
  int bubbleCount; /*!< Bubbles left to spawn */
  bool leaveState; /*!< Flag to switch to next state */
  
public:
  StarfishAttackState(int maxBubbleCount);
  ~StarfishAttackState();

  /**
   * @brief Adds DoAttack() callbacks for animations and animates
   * @param star
   */
  void OnEnter(Starfish& star);
  
  /**
   * @brief Does nothing
   * @param _elapsed
   * @param star
   */
  void OnUpdate(float _elapsed, Starfish& star);
  
  /**
   * @brief Does nothing
   * @param star
   */
  void OnLeave(Starfish& star);
  
  /**
   * @brief Spawns bubbles until count reaches zero then sets to StarfishIdleState
   * @param star
   * 
   * When the bubble count is zero, removes all callbacks in the animation
   * and adds a new callback when the attack animation finishes to change 
   * into StarfishIdleState
   */
  void DoAttack(Starfish& star);
};

