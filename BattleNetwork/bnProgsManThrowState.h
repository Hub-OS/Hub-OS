#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManThrowState : public AIState<ProgsMan>
{
public:
  ProgsManThrowState();
  ~ProgsManThrowState();

  void OnEnter(ProgsMan& progs);
  void OnUpdate(float _elapsed, ProgsMan& progs);
  void OnLeave(ProgsMan& progs);
};

