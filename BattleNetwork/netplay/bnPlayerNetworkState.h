#pragma once
#include "../bnAIState.h"
#include "battlescene/bnNetworkBattleScene.h"
#include "bnNetPlayFlags.h"

class PlayerNetworkState : public AIState<Player>
{
private:
  bool isChargeHeld{}; /*!< Flag if player is holding down shoot button */
  NetPlayFlags& netflags;
public:

  /**
   * @brief isChargeHeld is set to false
   */
  PlayerNetworkState(NetPlayFlags&);

  /**
   * @brief deconstructor
   */
  ~PlayerNetworkState();

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
};