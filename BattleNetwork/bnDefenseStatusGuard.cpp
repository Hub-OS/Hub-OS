#include "bnDefenseStatusGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnGuardHit.h"

DefenseStatusGuard::DefenseStatusGuard() : DefenseRule(Priority(1), DefenseOrder::always)
{
}

DefenseStatusGuard::~DefenseStatusGuard()
{
}

Hit::Properties& DefenseStatusGuard::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~(Hit::bubble | Hit::freeze | Hit::stun | Hit::root);
  return statuses;
}

void DefenseStatusGuard::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{ }
