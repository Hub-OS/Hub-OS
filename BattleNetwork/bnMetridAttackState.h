#pragma once
#include "bnAIState.h"

class Metrid;
 
namespace Battle {
  class Tile;
}

/*! \brief Sets animation to attack and adds callback to spawn wave
 */
class MetridAttackState : public AIState<Metrid>
{
private:
  int meteorCount;
  float meteorCooldown;
  bool beginAttack;
  Battle::Tile* target;

public:

  /**
   * @brief constructor
   */
  MetridAttackState();

  /**
   * @brief deconstructor
   */
  ~MetridAttackState();

  /**
   * @brief Sets animation and adds callback to fire DoAttack()
   * @param met
   */
  void OnEnter(Metrid& met);

  /**
   * @brief Does nothing
   * @param _elapsed
   * @param met
   */
  void OnUpdate(float _elapsed, Metrid& met);

  /**
   * @brief Does nothing
   * @param met
   */
  void OnLeave(Metrid& met);

  /**
   * @brief animates before spawning a series of meteors
   * @param met
   * Also adds callback to change state
   */
  void DoAttack(Metrid& met);
};

