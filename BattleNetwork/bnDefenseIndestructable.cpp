#include "bnDefenseIndestructable.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseIndestructable::DefenseIndestructable(bool breakCollidingObjectOnHit) : breakCollidingObjectOnHit(breakCollidingObjectOnHit), DefenseRule(Priority(1))
{
}

DefenseIndestructable::~DefenseIndestructable()
{
}

void DefenseIndestructable::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  arbiter.BlockImpact();

  // Only drop gaurd effect as a response to attacks that can do impact damage > 0
  if (in.GetHitboxProperties().damage > 0 && (in.GetHitboxProperties().flags & Hit::impact) != 0) {
    owner.GetField()->AddEntity(*new GuardHit(owner.GetField(), &owner, true), *owner.GetTile());
    arbiter.BlockDamage();
  }

  if (breakCollidingObjectOnHit) in.Delete();
}
