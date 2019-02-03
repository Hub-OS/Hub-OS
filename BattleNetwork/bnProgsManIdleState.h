#pragma once
#include "bnAIState.h"
class ProgsMan;

class ProgsManIdleState : public AIState<ProgsMan>
{
private:
  float cooldown;

public:
  ProgsManIdleState();
  ~ProgsManIdleState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
};

