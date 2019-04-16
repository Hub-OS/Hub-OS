#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManShootState : public AIState<ProgsMan>
{
public:
  ProgsManShootState();
  ~ProgsManShootState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
};

