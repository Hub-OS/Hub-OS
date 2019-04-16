#pragma once
#include "bnAIState.h"
class MetalMan;

class MetalManIdleState : public AIState<MetalMan>
{
private:
  float cooldown;

public:
  MetalManIdleState();
  ~MetalManIdleState();

  void OnEnter(MetalMan& p);
  void OnUpdate(float _elapsed, MetalMan& p);
  void OnLeave(MetalMan& p);
};

#pragma once
