#pragma once
#include "bnAIState.h"
#include "bnStarfish.h"

class StarfishAttackState : public AIState<Starfish>
{
  int bubbleCount;

public:
  StarfishAttackState(int maxBubbleCount);
  ~StarfishAttackState();

  void OnEnter(Starfish& star);
  void OnUpdate(float _elapsed, Starfish& star);
  void OnLeave(Starfish& star);
  void DoAttack(Starfish& star);
};

