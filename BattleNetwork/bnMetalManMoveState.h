#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

class MetalManMoveState : public AIState<MetalMan>
{
private:
  Direction nextDirection;
  bool isMoving;
  bool mustMove;
public:
  MetalManMoveState();
  ~MetalManMoveState();

  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
  void OnLeave(MetalMan& m);
};

