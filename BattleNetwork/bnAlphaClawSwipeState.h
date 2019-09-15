#pragma once
#include "bnAIState.h"

class AlphaCore;
class AlphaArm;

class AlphaClawSwipeState : public AIState<AlphaCore>
{
private:
  AlphaArm *leftArm, *rightArm;

public:
  AlphaClawSwipeState();
  ~AlphaClawSwipeState();

  void OnEnter(AlphaCore& a);
  void OnUpdate(float _elapsed, AlphaCore& a);
  void OnLeave(AlphaCore& a);
};
