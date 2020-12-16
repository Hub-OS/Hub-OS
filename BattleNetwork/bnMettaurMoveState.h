#pragma once
#include "bnAIState.h"
#include "bnMettaur.h"
#include "bnTile.h"
#include "bnField.h"

/**
 * @class MettaurMoveState
 * @author mav
 * @date 28/04/19
 * @brief Moves mettaur and decides to attack or pass
 */
class MettaurMoveState : public AIState<Mettaur>
{
private:
  Direction nextDirection; /*!< Direction to move to */
  bool isMoving; /*!< Whether or not move animation is playing */
public:

  /**
   * @brief constructor
   */
  MettaurMoveState();
  
  /**
   * @brief deconstructor
   */
  ~MettaurMoveState();

  /**
   * @brief does nothing
   * @param met
   */
  void OnEnter(Mettaur& met);
  
  /**
   * @brief Tries to move to player. If it can't passes turn
   * @param _elapsed in seconds
   * @param met
   */
  void OnUpdate(double _elapsed, Mettaur& met);
  
  /**
   * @brief Does nothing
   * @param met
   */
  void OnLeave(Mettaur& met);
};

