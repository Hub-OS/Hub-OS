#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

class MetalManThrowState : public AIState<MetalMan>
{
public:
  MetalManThrowState();
  ~MetalManThrowState();

  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
  void OnLeave(MetalMan& m);
  void Attack(MetalMan& m);
};