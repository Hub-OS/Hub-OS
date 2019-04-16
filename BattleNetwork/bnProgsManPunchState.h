#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManPunchState : public AIState<ProgsMan>
{
public:
  ProgsManPunchState();
  ~ProgsManPunchState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
  void Attack(ProgsMan& p);
};

