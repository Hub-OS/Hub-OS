#include "bnDefenseResolutionArbiter.h"

const bool DefenseResolutionArbiter::IsDamageBlocked() const
{
  return this->blockedDamage;
}

const bool DefenseResolutionArbiter::IsImpactBlocked() const
{
  return this->blockedImpact;
}

void DefenseResolutionArbiter::BlockDamage()
{
  this->blockedDamage = true;
}

void DefenseResolutionArbiter::BlockImpact()
{
  this->blockedImpact = true;
}

void DefenseResolutionArbiter::ExecuteAllTriggers()
{
  for (auto&& trigger : triggers) {
    trigger();
  }

  triggers.clear();
}
