#pragma once
#include "bnAIState.h"
#include "bnStarfish.h"

class StarfishIdleState : public AIState<Starfish>
{
  float cooldown;
public:
  StarfishIdleState();
  ~StarfishIdleState();

  void OnEnter(Starfish& star);
  void OnUpdate(float _elapsed, Starfish& star);
  void OnLeave(Starfish& star);
};

