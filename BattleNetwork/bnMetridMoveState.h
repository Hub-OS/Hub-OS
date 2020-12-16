#pragma once
#include "bnAIState.h"
#include "bnTile.h"
#include "bnField.h"

class Metrid;
 
/**
 * @class MetridMoveState
 * @author mav
 * @date 28/04/19
 * @brief Moves metrid and decides to attack after 5 places
 */
class MetridMoveState : public AIState<Metrid>
{
private:
  bool isMoving; /*!< Whether or not move animation is playing */
  int moveCount; /*!< 5 counts down to 0 before attacking*/
  float cooldown; /*!< wait before moving again*/
public:

  /**
   * @brief constructor
   */
  MetridMoveState();

  /**
   * @brief deconstructor
   */
  ~MetridMoveState();

  /**
   * @brief does nothing
   * @param met
   */
  void OnEnter(Metrid& met);

  /**
   * @brief Moves 5 times before attacking
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

