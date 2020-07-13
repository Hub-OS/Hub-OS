#pragma once
#include "../bnAIState.h"

struct NetPlayFlags {
  bool isRemotePlayerLoser{ false };
  bool isRemoteConnected{ false };
  bool isRemoteReady{ false };
  bool remoteShoot{ false };
  bool remoteUseSpecial{ false };
  bool remoteCharge{ false };
  int remoteHP{ 1 };
  int remoteTileX{ 5 }, remoteTileY{ 2 };
  int remoteFormSelect{ 0 };
  Direction remoteDirection{ Direction::none };
  int remoteNavi;
  std::string remoteChipUse{ "" };
};

/*! \brief in this state navi can be controlled by the network */

class Tile;
class Player;
class InputManager;
class CardAction;

class PlayerNetworkState : public AIState<Player>
{
private:
  bool isChargeHeld; /*!< Flag if player is holding down shoot button */
  CardAction* queuedAction; /*!< Movement takes priority. If there is an action queued, fire on next best frame*/
  NetPlayFlags& netflags;
  void QueueAction(Player& player);
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
  void OnUpdate(float _elapsed, Player& player);

  /**
   * @brief Sets player entity charge component to false
   * @param player player entity
   */
  void OnLeave(Player& player);
};