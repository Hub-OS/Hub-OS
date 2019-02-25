#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"
#include "bnHitBox.h"

DefenseAura::DefenseAura() : DefenseRule(Priority(100))
{
}

DefenseAura::~DefenseAura()
{
}

const bool DefenseAura::Check(Spell * in, Character* owner)
{
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());

  return true; // barrier never lets attacks passthrough
}
