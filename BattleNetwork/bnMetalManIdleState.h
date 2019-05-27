#pragma once
#include "bnAIState.h"
class MetalMan;

<<<<<<< HEAD
class MetalManIdleState : public AIState<MetalMan>
{
private:
  float cooldown;
public:
  MetalManIdleState(float cooldown = 0.8f);
  ~MetalManIdleState();

  void OnEnter(MetalMan& p);
  void OnUpdate(float _elapsed, MetalMan& p);
  void OnLeave(MetalMan& p);
};

#pragma once
=======
/*! \brief Metalman's idle state waits before moving */
class MetalManIdleState : public AIState<MetalMan>
{
private:
  float cooldown; /*!< Timer to change states */

public:
  /**
   * @brief Sets cooldown to 0.8 seconds
   */
  MetalManIdleState();
  
  /**
   * @brief deconstructor
   */
  ~MetalManIdleState();

  /**
   * @brief Sets metalman's state to IDLE
   * @param m metalman
   */
  void OnEnter(MetalMan& m);
  
  /**
   * @brief When cooldown reaches zero changes to MetalManMoveState
   * @param _elapsed
   * @param m
   */
  void OnUpdate(float _elapsed, MetalMan& m);
  
  /**
   * @brief Does nothing
   * @param m
   */
  void OnLeave(MetalMan& m);
};
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
