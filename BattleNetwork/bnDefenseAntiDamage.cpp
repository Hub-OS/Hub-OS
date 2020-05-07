#include "bnDefenseAntiDamage.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"

DefenseAntiDamage::DefenseAntiDamage(const DefenseAntiDamage::Callback& callback) : callback(callback), DefenseRule(Priority(5))
{
}

DefenseAntiDamage::~DefenseAntiDamage()
{
}

const bool DefenseAntiDamage::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  auto props = in.GetHitboxProperties();

  if ((props.flags & Hit::impact) == Hit::impact && props.damage >= 10
    && !arbiter.IsDamageBlocked() && !arbiter.IsImpactBlocked()) {

    owner.GetField()->AddEntity(*new Hitbox(owner.GetField(), owner.GetTeam(), 0), owner.GetTile()->GetX(), owner.GetTile()->GetY());

    arbiter.AddTrigger(callback, std::ref(in), std::ref(owner));
    arbiter.BlockDamage();

    return true; // Antidamage disallows an attack to passthrough
  }

  return false;
}
