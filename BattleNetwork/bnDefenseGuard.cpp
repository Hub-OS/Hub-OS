#include "bnDefenseGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseGuard::DefenseGuard(const DefenseGuard::Callback& callback) : callback(callback), DefenseRule(Priority(1))
{
}

DefenseGuard::~DefenseGuard()
{
}

void DefenseGuard::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  auto props = in.GetHitboxProperties();

  if ((props.flags & Hit::breaking) == 0) {
    arbiter.AddTrigger(callback, std::ref(in), std::ref(owner));
    arbiter.BlockDamage();

    if ((props.flags & Hit::impact) == Hit::impact) {
      arbiter.BlockImpact();
      owner.GetField()->AddEntity(*new GuardHit(owner.GetField(), &owner, true), owner.GetTile()->GetX(), owner.GetTile()->GetY());
      owner.GetField()->AddEntity(*new Hitbox(owner.GetField(), owner.GetTeam(), 0), owner.GetTile()->GetX(), owner.GetTile()->GetY());
    }
  }
}
