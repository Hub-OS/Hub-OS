#pragma once
#include "bnAIState.h"

class Metrid;

class MetridIdleState : public AIState<Metrid>
{
  double cooldown; /*!< How long to wait before moving */
public:

  /**
   * @brief Sets default cooldown to 1 second
   */
  MetridIdleState();

  /**
   * @brief deconstructor
   */
  ~MetridIdleState();

  /**
   * @brief Sets animation to correct rank color met
   * @param met
   *
   * If SP met, cooldown is set to 0.5 seconds
   */
  void OnEnter(Metrid& met);

  /**
   * @brief When cooldown hits zero, changes to MetridMoveState
   * @param _elapsed in seconds
   * @param met
   */
  void OnUpdate(double _elapsed, Metrid& met);

  /**
   * @brief Does nothing
   * @param met
   */
  void OnLeave(Metrid& met);
};

