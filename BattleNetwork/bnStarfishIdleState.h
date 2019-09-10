
/*! \brief Idle state for Starfish AI
 * 
 * Animators in place until the timer runs down 
 * and then switches to Attack state
 */
#pragma once
#include "bnAIState.h"

class Starfish;

class StarfishIdleState : public AIState<Starfish>
{

  float cooldown; /*!< Seconds before moving to next state */
public:
  StarfishIdleState();
  ~StarfishIdleState();

  /**
   * @brief Sets the starfish animation to IDLE
   * @param star the agent to update
   */
  void OnEnter(Starfish& star);
  
  /**
   * @brief Animators in place before switching to StarfishAttackState
   * @param _elapsed in seconds
   * @param star the agent to update
   */
  void OnUpdate(float _elapsed, Starfish& star);
  
  /**
   * @brief Does nothing
   * @param star
   */
  void OnLeave(Starfish& star);
};

