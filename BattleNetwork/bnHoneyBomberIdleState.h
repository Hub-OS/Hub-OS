#pragma once
#include "bnAIState.h"

class HoneyBomber;

class HoneyBomberIdleState : public AIState<HoneyBomber>
{
  float cooldown; /*!< How long to wait before moving */
public:

  /**
   * @brief Sets default cooldown to 1 second
   */
  HoneyBomberIdleState();

  /**
   * @brief deconstructor
   */
  ~HoneyBomberIdleState();

  /**
   * @brief Sets animation to correct rank color
   * @param honey
   *
   * If SP honey, cooldown is set to 0.5 seconds
   */
  void OnEnter(HoneyBomber& honey);

  /**
   * @brief When cooldown hits zero, changes to HoneyBomberMoveState
   * @param _elapsed in seconds
   * @param honey
   */
  void OnUpdate(double _elapsed, HoneyBomber& honey);

  /**
   * @brief Does nothing
   * @param honey
   */
  void OnLeave(HoneyBomber& honey);
};

