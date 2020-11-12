#include "bnDefenseStatusGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseStatusGuard::DefenseStatusGuard() : DefenseRule(Priority(1), DefenseOrder::always)
{
}

DefenseStatusGuard::~DefenseStatusGuard()
{
}

Hit::Properties& DefenseStatusGuard::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~(Hit::bubble | Hit::freeze | Hit::stun);
  return statuses;
}

void DefenseStatusGuard::CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner)
{ }
