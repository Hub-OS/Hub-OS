#pragma once
#include "bnAIState.h"

class AlphaCore;

class AlphaGunState : public AIState<AlphaCore>
{
private:
  // in seconds
  float cooldown;
  int count;
  Battle::Tile* last;
public:
  AlphaGunState();
  ~AlphaGunState();

  void OnEnter(AlphaCore& a) override;
  void OnUpdate(float _elapsed, AlphaCore& a) override;
  void OnLeave(AlphaCore& a) override;
};
