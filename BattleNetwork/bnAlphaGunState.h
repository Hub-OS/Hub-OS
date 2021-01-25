#pragma once
#include "bnAIState.h"

class AlphaCore;

class AlphaGunState : public AIState<AlphaCore>
{
private:
  // in seconds
  double cooldown{};
  int count{};
  Battle::Tile* last{ nullptr };
public:
  AlphaGunState();
  ~AlphaGunState();

  void OnEnter(AlphaCore& a) override;
  void OnUpdate(double _elapsed, AlphaCore& a) override;
  void OnLeave(AlphaCore& a) override;
};
