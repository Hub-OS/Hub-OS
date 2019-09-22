#pragma once
#include "bnAIState.h"

class AlphaCore;

class AlphaRocketState : public AIState<AlphaCore>
{
private:
  // in seconds
  float cooldown;
  bool launched;
public:
  AlphaRocketState();
  ~AlphaRocketState();

  void OnEnter(AlphaCore& a);
  void OnUpdate(float _elapsed, AlphaCore& a);
  void OnLeave(AlphaCore& a);
};
