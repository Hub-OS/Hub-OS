#include "bnDefenseFrameStateJudge.h"
#include "bnDefenseRule.h"

DefenseFrameStateJudge::DefenseFrameStateJudge()
  : blockedDamage(false), blockedImpact(false), context(nullptr)
{
}

const bool DefenseFrameStateJudge::IsDamageBlocked() const
{
  return blockedDamage;
}

const bool DefenseFrameStateJudge::IsImpactBlocked() const
{
  return blockedImpact;
}

void DefenseFrameStateJudge::BlockDamage()
{
  blockedDamage = true;
}

void DefenseFrameStateJudge::BlockImpact()
{
  blockedImpact = true;
}

void DefenseFrameStateJudge::SignalDefenseWasPierced()
{
  if (context == nullptr) return;

  piercedSet.insert(piercedSet.begin(), context);

  auto iter = triggers.find(context);
  if (iter != triggers.end()) {
    triggers.erase(iter);
  }
}

void DefenseFrameStateJudge::SetDefenseContext(DefenseRule* ptr)
{
  context = ptr;
}

void DefenseFrameStateJudge::PrepareForNextAttack()
{
  blockedDamage = blockedImpact = false;
  context = nullptr;
}

void DefenseFrameStateJudge::ExecuteAllTriggers()
{
  for (auto&& [key, closure] : triggers) {
    closure();
  }

  triggers.clear();
}
