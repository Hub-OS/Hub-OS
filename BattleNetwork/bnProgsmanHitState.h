#pragma once
#include "bnAIState.h"
class ProgsMan;

class ProgsManHitState : public AIState<ProgsMan>
{
private:
  float cooldown;

public:
  ProgsManHitState();
  ~ProgsManHitState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
};

