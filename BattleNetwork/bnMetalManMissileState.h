#pragma once
#include "bnAIState.h"
class MetalMan;

class MetalManMissileState : public AIState<MetalMan>
{
private:
  double cooldown;
  double lastMissileTimestamp;
  int missiles, missileIndex;

public:
    MetalManMissileState(int missiles);
  ~MetalManMissileState();

  void OnEnter(MetalMan& p);
  void OnUpdate(double _elapsed, MetalMan& p);
  void OnLeave(MetalMan& p);
};