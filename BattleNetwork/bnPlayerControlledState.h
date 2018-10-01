#pragma once
#include "bnAIState.h"
class Tile;
class Player;
class InputManager;

class PlayerControlledState : public AIState<Player>
{
private:
  InputManager * inputManager;

  //Cooldowns
  float moveKeyPressCooldown;
  float attackKeyPressCooldown;
  float attackToIdleCooldown;

  bool isChargeHeld;

  Direction previousDirection;

public:
  PlayerControlledState();
  ~PlayerControlledState();


  void OnEnter(Player& player);
  void OnUpdate(float _elapsed, Player& player);
  void OnLeave(Player& player);
};

