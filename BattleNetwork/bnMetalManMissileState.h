#pragma once
#include "bnAIState.h"
class MetalMan;

class MetalManMissileState : public AIState<MetalMan>
{
private:
  float cooldown;
  float lastMissileTimestamp;
  int missiles;

public:
    MetalManMissileState(int missiles = 0);
  ~MetalManMissileState();

  void OnEnter(MetalMan& p);
  void OnUpdate(float _elapsed, MetalMan& p);
  void OnLeave(MetalMan& p);
};