#include "bnDefenseIndestructable.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitBox.h"
#include "bnGuardHit.h"

DefenseIndestructable::DefenseIndestructable() : DefenseRule(Priority(1))
{
}

DefenseIndestructable::~DefenseIndestructable()
{
}

const bool DefenseIndestructable::Check(Spell * in, Character* owner)
{
  owner->GetField()->AddEntity(*new GuardHit(owner->GetField(), owner, true), owner->GetTile()->GetX(), owner->GetTile()->GetY());
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());

  return true; // Guard disallows an attack to passthrough
}
