#pragma once
#include "bnAIState.h"

class Megalian;

class MegalianIdleState : public AIState<Megalian>
{
  float cooldown; /*!< How long to wait before moving */
public:

  /**
   * @brief Sets default cooldown to 1 second
   */
  MegalianIdleState();

  /**
   * @brief deconstructor
   */
  ~MegalianIdleState();

  /**
   * @brief Sets animation to correct rank color met
   * @param met
   *
   * If SP met, cooldown is set to 0.5 seconds
   */
  void OnEnter(Megalian& m);

  /**
   * @brief When cooldown hits zero, changes to MettaurMoveState
   * @param _elapsed in seconds
   * @param met
   */
  void OnUpdate(double _elapsed, Megalian& m);

  /**
   * @brief Does nothing
   * @param met
   */
  void OnLeave(Megalian& m);
};

