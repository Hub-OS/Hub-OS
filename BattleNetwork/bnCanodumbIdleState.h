#pragma once
#include "bnAIState.h"

class CanodumbCursor;
class Canodumb;

/**
 * @class CanodumbIdleState
 * @author mav
 * @date 05/05/19
 * @brief Spawns a cursor and waits until cursor finds an enemy
 */
class CanodumbIdleState : public AIState<Canodumb>
{
  CanodumbCursor* cursor; /*!< Spawned to find enemies to attack */

public:
  CanodumbIdleState();
  ~CanodumbIdleState();

  /**
   * @brief Sets idle animation based on rank
   * @param can canodumb
   */
  void OnEnter(Canodumb& can);
  
  /**
   * @brief If no cursor exists, spawns one to look for enemies
   * @param _elapsed in seconds
   * @param can canodumb
   */
  void OnUpdate(float _elapsed, Canodumb& can);
  
  /**
   * @brief Deletes any existing cursors 
   * @param can canodumb 
   */
  void OnLeave(Canodumb& can);
};

