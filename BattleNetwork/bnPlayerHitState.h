/*! \brief Player is hit and sets the invincibility timer */

#pragma once
#include "bnAI.h"
class Player;
class PlayerHitState : public AIState<Player>
{
public:

  /**
   * @brief constructor
   */
  PlayerHitState();
  
  /**
   * @brief deconstructor
   */
  ~PlayerHitState();

  /**
   * @brief Sets hit animation. On end, changes to PlayerIdleState
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
   * @brief Sets player's invincibility timer to 2 seconds
   * @param player player entity
   */
  void OnLeave(Player& player);
};

