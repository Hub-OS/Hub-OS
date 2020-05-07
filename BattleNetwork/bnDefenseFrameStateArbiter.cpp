#include "bnDefenseFrameStateArbiter.h"

const bool DefenseFrameStateArbiter::IsDamageBlocked() const
{
  return blockedDamage;
}

const bool DefenseFrameStateArbiter::IsImpactBlocked() const
{
  return blockedImpact;
}

void DefenseFrameStateArbiter::BlockDamage()
{
  blockedDamage = true;
}

void DefenseFrameStateArbiter::BlockImpact()
{
  blockedImpact = true;
}

void DefenseFrameStateArbiter::ExecuteAllTriggers()
{
  for (auto&& trigger : triggers) {
    trigger();
  }

  triggers.clear();
}
