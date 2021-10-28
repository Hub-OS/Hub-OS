#include "bnDefenseIndestructable.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnGuardHit.h"

DefenseIndestructable::DefenseIndestructable(bool breakCollidingObjectOnHit)
  : breakCollidingObjectOnHit(breakCollidingObjectOnHit), DefenseRule(Priority(1), DefenseOrder::always)
{
}

DefenseIndestructable::~DefenseIndestructable()
{
}

Hit::Properties& DefenseIndestructable::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~(Hit::flash|Hit::freeze|Hit::stun|Hit::flinch|Hit::pierce|Hit::shake|Hit::root);
  return statuses;
}

void DefenseIndestructable::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{
  judge.BlockImpact();

  // Only drop gaurd effect as a response to attacks that can do impact damage > 0
  if (attacker->GetHitboxProperties().damage > 0 && (attacker->GetHitboxProperties().flags & Hit::impact) != 0) {
    owner->GetField()->AddEntity(std::make_shared<GuardHit>(owner, true), *owner->GetTile());
    judge.BlockDamage();
  }

  if (breakCollidingObjectOnHit) attacker->Delete();
}
