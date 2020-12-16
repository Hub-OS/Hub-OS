#pragma once
#include "bnAIState.h"
class Player;

class PlayerChangeFormState : public AIState<Player>
{
private:
  int nextForm;
  std::function<void()> callback;

public:
  PlayerChangeFormState(int formAt, std::function<void()> onEnd);
  ~PlayerChangeFormState();

  void OnEnter(Player& player);
  void OnUpdate(double _elapsed, Player& player);
  void OnLeave(Player& player);
};

