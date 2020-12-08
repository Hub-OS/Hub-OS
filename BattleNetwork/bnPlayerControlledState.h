/*! \brief in this state navi can be controlled by the player */

#pragma once
#include "bnAIState.h"
#include "bnInputHandle.h"

class Tile;
class Player;
class InputManager;
class CardAction;
class PlayerInputReplicator;

constexpr const float STARTUP_DELAY_LEN { 5.0f / 60.f }; // 5 frame startup delay out of 60fps

class PlayerControlledState : public AIState<Player>, public InputHandle
{
private:  
  bool isChargeHeld; /*!< Flag if player is holding down shoot button */
  CardAction* queuedAction; /*!< Movement takes priority. If there is an action queued, fire on next best frame*/
  PlayerInputReplicator* replicator; /*!< Pass actions onto a replicator to handle if requested */
  float startupDelay{ 0 };

  void QueueAction(Player& player);

public:

  /**
   * @brief isChargeHeld is set to false
   */
  PlayerControlledState();
  
  /**
   * @brief deconstructor
   */
  ~PlayerControlledState();

  /**
   * @brief Sets animation to IDLE
   * @param player player entity
   */
  void OnEnter(Player& player);
  
  /**
   * @brief Listens to input and manages shooting and moving animations
   * @param _elapsed
   * @param player
   */
  void OnUpdate(float _elapsed, Player& player);
  
  /**
   * @brief Sets player entity charge component to false
   * @param player player entity
   */
  void OnLeave(Player& player);
};

