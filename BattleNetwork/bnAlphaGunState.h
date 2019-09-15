#pragma once
#include "bnAIState.h"

class AlphaCore;

class AlphaGunState : public AIState<AlphaCore>
{
private:
  // in seconds
  float cooldown;

public:
  AlphaGunState();
  ~AlphaGunState();

  void OnEnter(AlphaCore& a);
  void OnUpdate(float _elapsed, AlphaCore& a);
  void OnLeave(AlphaCore& a);
};
