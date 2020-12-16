/*! \brief Progsman idlestate has a cooldown timer before moving */

#pragma once
#include "bnAIState.h"
class ProgsMan;

class ProgsManIdleState : public AIState<ProgsMan>
{
private:
  float cooldown; /*!< Time left before changing states */
  float initialCooldown; /*!< Cooldown used in this state's constructor */
public:
  /**
   * @brief Sets the timer to 0.5 seconds
   */
  ProgsManIdleState(const float);
  
  /**
   * @brief Deconstructor
   */
  ~ProgsManIdleState();

  /**
   * @brief Set the animation to IDLE
   * @param p progsman entity
   */
  void OnEnter(ProgsMan& p);
  
  /**
   * @brief After timer runs out change to ProgsManMoveState
   * @param _elapsed in seconds
   * @param p progsman entity
   */
  void OnUpdate(double _elapsed, ProgsMan& p);
  
  /**
   * @brief Does nothing
   * @param p progsman entity
   */
  void OnLeave(ProgsMan& p);
};

