#pragma once
#include "bnAIState.h"
#include "bnCharacter.h"
#include "bnCanodumbCursor.h"

class Canodumb;

/**
 * @class CanodumbIdleState
 * @author mav
 * @date 05/05/19
 * @brief Spawns a cursor and waits until cursor finds an enemy
 */
class CanodumbIdleState : public AIState<Canodumb>
{
private:
  CanodumbCursor* cursor; /*!< Spawned to find enemies to attack */
  Canodumb* can;
  Entity::DeleteCallback freeCursorCallback;
  friend void CanodumbCursor::OnUpdate(float _elapsed);
  friend CanodumbCursor::CanodumbCursor(Field* _field, Team _team, CanodumbIdleState* _parent);

  void Attack();
  void FreeCursor();
  Character::Rank GetCanodumbRank();
  Entity* GetCanodumbTarget();
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

