#pragma once
#include "bnAIState.h"
#include "bnHoneyBomber.h"

namespace Battle {
  class Tile;
}


class HoneyBomberAttackState : public AIState<HoneyBomber>
{
private:
  int beeCount;
  float attackCooldown;
public:

  /**
   * @brief constructor
   */
  HoneyBomberAttackState();

  /**
   * @brief deconstructor
   */
  ~HoneyBomberAttackState();

  /**
   * @brief Sets animation and adds callback to fire DoAttack()
   * @param honey
   */
  void OnEnter(HoneyBomber& honey);

  /**
   * @brief Does nothing
   * @param _elapsed
   * @param honey
   */
  void OnUpdate(float _elapsed, HoneyBomber& honey);

  /**
   * @brief Does nothing
   * @param honey
   */
  void OnLeave(HoneyBomber& honey);

  /**
   * @brief animates before spawning a series of bees
   * @param honey
   * Also adds callback to change state
   */
  void DoAttack(HoneyBomber& honey);
};

