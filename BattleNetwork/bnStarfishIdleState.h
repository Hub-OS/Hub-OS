<<<<<<< HEAD
=======
/*! \brief Idle state for Starfish AI
 * 
 * Animates in place until the timer runs down 
 * and then switches to Attack state
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnAIState.h"
#include "bnStarfish.h"

class StarfishIdleState : public AIState<Starfish>
{
<<<<<<< HEAD
  float cooldown;
=======
  float cooldown; /*!< Seconds before moving to next state */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
public:
  StarfishIdleState();
  ~StarfishIdleState();

<<<<<<< HEAD
  void OnEnter(Starfish& star);
  void OnUpdate(float _elapsed, Starfish& star);
=======
  /**
   * @brief Sets the starfish animation to IDLE
   * @param star the agent to update
   */
  void OnEnter(Starfish& star);
  
  /**
   * @brief Animates in place before switching to StarfishAttackState
   * @param _elapsed in seconds
   * @param star the agent to update
   */
  void OnUpdate(float _elapsed, Starfish& star);
  
  /**
   * @brief Does nothing
   * @param star
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void OnLeave(Starfish& star);
};

