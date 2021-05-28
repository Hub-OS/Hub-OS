/*! \brief in this state navi can be controlled by the player */

#pragma once
#include "bnAIState.h"
#include "bnInputHandle.h"
#include <vector>
#include <chrono>

class Tile;
class Player;
class InputManager;
class CardAction;
class PlayerInputReplicator;

class PlayerControlledState : public AIState<Player>, public InputHandle
{
private:  
  bool isChargeHeld{}; /*!< Flag if player is holding down shoot button */
  unsigned moveFrame{};
  double currLag{};
  PlayerInputReplicator* replicator{ nullptr }; /*!< Pass actions onto a replicator to handle if requested */
  std::vector<InputEvent> inputQueue;

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
  void OnUpdate(double _elapsed, Player& player);
  
  /**
   * @brief Sets player entity charge component to false
   * @param player player entity
   */
  void OnLeave(Player& player);

  const bool InputQueueHas(const InputEvent& item);
  void InputQueueCleanup();
};

