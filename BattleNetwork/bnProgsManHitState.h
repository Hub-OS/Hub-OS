<<<<<<< HEAD
=======
/*! \brief Progsman flinches for 0.5 seconds */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnAIState.h"
class ProgsMan;

class ProgsManHitState : public AIState<ProgsMan>
{
private:
<<<<<<< HEAD
  float cooldown;

public:
  ProgsManHitState();
  ~ProgsManHitState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
=======
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
  void OnUpdate(float _elapsed, ProgsMan& p);
  
  /**
   * @brief Does nothing
   * @param p progsman entity
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void OnLeave(ProgsMan& p);
};

