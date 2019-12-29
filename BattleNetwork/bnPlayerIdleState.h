/*! \brief The Idle state for the player */

#pragma once
#include "bnAIState.h"
class Player;

class PlayerIdleState : public AIState<Player>
{
public:
  PlayerIdleState();
  ~PlayerIdleState();

  /**
   * @brief sets the animation to IDLE
   * @param player player entity
   */
  void OnEnter(Player& player);
  
  /**
   * @brief does nothing
   * @param _elapsed
   * @param player player entity
   */
  void OnUpdate(float _elapsed, Player& player);
  
  /**
   * @brief does nothing
   * @param player player entity
   */
  void OnLeave(Player& player);
};

