#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"
#include "bnHitBox.h"

DefenseAura::DefenseAura(DefenseAura::Callback callback) : DefenseRule(Priority(4))
{
	this->callback = callback;
}

DefenseAura::DefenseAura() : DefenseRule(Priority(4)) {
	callback = nullptr;
}

DefenseAura::~DefenseAura()
{
}

const bool DefenseAura::Check(Spell * in, Character* owner)
{
  // Drop a 0 damage hitbox to block/trigger attack hits
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());

  if(callback) { callback(in, owner); }
  
  return true; // barrier never lets attacks passthrough
}
