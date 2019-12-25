/*! \brief in this state navi can be controlled by the player */

#pragma once
#include "bnAIState.h"
class Tile;
class Player;
class InputManager;
class ChipAction;

class PlayerControlledState : public AIState<Player>
{
private:  
  bool isChargeHeld; /*!< Flag if player is holding down shoot button */
  ChipAction* queuedAction; /*!< Movement takes priority. If there is an action queued, fire on next best frame*/

  const bool CanTakeAction(Player& player) const;

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

