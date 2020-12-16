
/*! \brief Progsman flinches for 0.5 seconds */

#pragma once
#include "bnAIState.h"
class ProgsMan;

class ProgsManHitState : public AIState<ProgsMan>
{
private:
  float cooldown; /*!< Time left before changing states */

public:
  /**
   * @brief Sets cooldown time to 0.5 seconds
   */
  ProgsManHitState();
  
  /**
   * @brief deconstructor
   */
  ~ProgsManHitState();

  /**
   * @brief Does nothing
   * @param p progsman entity
   */
  void OnEnter(ProgsMan& p);
  
  /**
   * @brief Cooldown timer reaches zero and changes to ProgsManIdleState
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

