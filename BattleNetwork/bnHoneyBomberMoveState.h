#pragma once
#include "bnAIState.h"
#include "bnHoneyBomber.h"
#include "bnTile.h"
#include "bnField.h"

/**
 * @class HoneyBomberMoveState
 * @author mav
 * @date 28/04/19
 * @brief Moves HoneyBomber and decides to attack after 3 places
 */
class HoneyBomberMoveState : public AIState<HoneyBomber>
{
private:
  bool isMoving; /*!< Whether or not move animation is playing */
  int moveCount; /*!< 4 counts down to 0 before attacking*/
  float cooldown; /*!< wait before moving again*/
public:

  /**
   * @brief constructor
   */
  HoneyBomberMoveState();

  /**
   * @brief deconstructor
   */
  ~HoneyBomberMoveState();

  /**
   * @brief does nothing
   * @param honey
   */
  void OnEnter(HoneyBomber& honey);

  /**
   * @brief Moves 3 times before attacking
   * @param _elapsed in seconds
   * @param honey
   */
  void OnUpdate(float _elapsed, HoneyBomber& honey);

  /**
   * @brief Does nothing
   * @param honey
   */
  void OnLeave(HoneyBomber& honey);
};

