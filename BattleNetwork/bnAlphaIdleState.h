#pragma once
#include "bnAIState.h"

class AlphaCore;

class AlphaIdleState : public AIState<AlphaCore>
{
private:
  // in seconds
  float cooldown;

public:
  AlphaIdleState();
  ~AlphaIdleState();

  void OnEnter(AlphaCore& a);
  void OnUpdate(float _elapsed, AlphaCore& a);
  void OnLeave(AlphaCore& a);
};
