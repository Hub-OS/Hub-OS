#include "bnDefenseIndestructable.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
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
  statuses.flags &= ~(Hit::flash|Hit::freeze|Hit::stun|Hit::flinch|Hit::pierce|Hit::shake);
  return statuses;
}

void DefenseIndestructable::CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner)
{
  judge.BlockImpact();

  // Only drop gaurd effect as a response to attacks that can do impact damage > 0
  if (in.GetHitboxProperties().damage > 0 && (in.GetHitboxProperties().flags & Hit::impact) != 0) {
    owner.GetField()->AddEntity(*new GuardHit(&owner, true), *owner.GetTile());
    judge.BlockDamage();
  }

  if (breakCollidingObjectOnHit) in.Delete();
}
