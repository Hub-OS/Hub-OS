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
class CanodumbIdleState final : public AIState<Canodumb>
{
private:
  std::shared_ptr<CanodumbCursor> cursor{ nullptr }; /*!< Spawned to find enemies to attack */
  Canodumb* can{ nullptr };
  friend void CanodumbCursor::OnUpdate(double _elapsed);
  friend CanodumbCursor::CanodumbCursor(CanodumbIdleState* _parent);

  void Attack();
  void FreeCursor();
  Character::Rank GetCanodumbRank();
  std::shared_ptr<Entity> GetCanodumbTarget();
public:
  CanodumbIdleState();
  ~CanodumbIdleState();

  /**
   * @brief Sets idle animation based on rank
   * @param can canodumb
   */
  void OnEnter(Canodumb& can) override;
  
  /**
   * @brief If no cursor exists, spawns one to look for enemies
   * @param _elapsed in seconds
   * @param can canodumb
   */
  void OnUpdate(double _elapsed, Canodumb& can) override;
  
  /**
   * @brief Deletes any existing cursors 
   * @param can canodumb 
   */
  void OnLeave(Canodumb& can) override;
};

