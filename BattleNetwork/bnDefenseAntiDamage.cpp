#include "bnDefenseAntiDamage.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitBox.h"
#include "bnParticlePoof.h"

DefenseAntiDamage::DefenseAntiDamage(DefenseAntiDamage::Callback callback) : callback(callback), DefenseRule(Priority(5))
{
}

DefenseAntiDamage::~DefenseAntiDamage()
{
}

const bool DefenseAntiDamage::Check(Spell * in, Character* owner)
{
  auto props = in->GetHitboxProperties();

  if ((props.flags & Hit::impact) == Hit::impact && props.damage > 10) {

    owner->GetField()->AddEntity(*new HitBox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());
    owner->GetField()->AddEntity(*new ParticlePoof(owner->GetField()), owner->GetTile()->GetX(), owner->GetTile()->GetY());

    this->callback(in, owner);

    return true; // Antidamage disallows an attack to passthrough
  }

  return false;
}
