#pragma once
#include "bnAIState.h"

class Mettaur;

class MettaurIdleState : public AIState<Mettaur>
{
  float cooldown; /*!< How long to wait before moving */
public:

  /**
   * @brief Sets default cooldown to 1 second
   */
  MettaurIdleState();
  
  /**
   * @brief deconstructor
   */
  ~MettaurIdleState();

  /**
   * @brief Sets animation to correct rank color met 
   * @param met
   * 
   * If SP met, cooldown is set to 0.5 seconds
   */
  void OnEnter(Mettaur& met);
  
  /**
   * @brief When cooldown hits zero, changes to MettaurMoveState
   * @param _elapsed in seconds
   * @param met
   */
  void OnUpdate(float _elapsed, Mettaur& met);
  
  /**
   * @brief Does nothing
   * @param met
   */
  void OnLeave(Mettaur& met);
};

