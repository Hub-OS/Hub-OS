#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

class MetalManPunchState : public AIState<MetalMan>
{
public:
  MetalManPunchState();
  ~MetalManPunchState();

  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
  void OnLeave(MetalMan& m);
  void Attack(MetalMan& m);
};

