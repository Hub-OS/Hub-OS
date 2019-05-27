#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"
#include "bnHitBox.h"

<<<<<<< HEAD
DefenseAura::DefenseAura() : DefenseRule(Priority(4))
{
=======
DefenseAura::DefenseAura(DefenseAura::Callback callback) : DefenseRule(Priority(4))
{
	this->callback = callback;
}

DefenseAura::DefenseAura() : DefenseRule(Priority(4)) {
	callback = nullptr;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

DefenseAura::~DefenseAura()
{
}

const bool DefenseAura::Check(Spell * in, Character* owner)
{
  owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());
<<<<<<< HEAD

=======
  if(callback) { callback(in, owner); }
  
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  return true; // barrier never lets attacks passthrough
}
