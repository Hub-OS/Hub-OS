#include "bnDefenseGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitboxSpell.h"
#include "bnGuardHit.h"

DefenseGuard::DefenseGuard(const DefenseGuard::Callback& callback)
  : callback(callback), DefenseRule(Priority(1), DefenseOrder::collisionOnly)
{
}

DefenseGuard::~DefenseGuard()
{
}

void DefenseGuard::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{
  auto props = attacker->GetHitboxProperties();

  if ((props.flags & Hit::breaking) == 0) {
    judge.BlockDamage();

    if ((props.flags & Hit::impact) == Hit::impact) {
      judge.AddTrigger(callback, attacker, owner);
      judge.BlockImpact();
      owner->GetField()->AddEntity(std::make_shared<GuardHit>(owner, true), *owner->GetTile());
      owner->GetField()->AddEntity(std::make_shared<HitboxSpell>(owner->GetTeam(), 0), *owner->GetTile());
    }
  }
  else if((props.flags & Hit::impact) == Hit::impact){
    judge.SignalDefenseWasPierced();
  }
}
