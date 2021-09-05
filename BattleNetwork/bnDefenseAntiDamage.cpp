#include "bnDefenseAntiDamage.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"

DefenseAntiDamage::DefenseAntiDamage(const DefenseAntiDamage::Callback& callback) : callback(callback), DefenseRule(Priority(5), DefenseOrder::collisionOnly)
{
}

DefenseAntiDamage::~DefenseAntiDamage()
{
}

void DefenseAntiDamage::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)
{
  auto props = in->GetHitboxProperties();

  if (props.element == Element::cursor) {
    judge.SignalDefenseWasPierced();
  } else if ((props.flags & Hit::impact) == Hit::impact && props.damage >= 10
    && !judge.IsDamageBlocked() && !judge.IsImpactBlocked()) {

    if (!triggering) {
        owner->GetField()->AddEntity(std::make_shared<Hitbox>(owner->GetTeam(), 0), *owner->GetTile());
        judge.AddTrigger(callback, in, owner);
    }

    judge.BlockDamage();
    triggering = true;
  }
}
