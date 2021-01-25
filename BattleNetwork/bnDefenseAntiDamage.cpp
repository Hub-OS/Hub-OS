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

void DefenseAntiDamage::CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner)
{
  auto props = in.GetHitboxProperties();

  if (props.element == Element::cursor) {
    judge.SignalDefenseWasPierced();
  } else if ((props.flags & Hit::impact) == Hit::impact && props.damage >= 10
    && !judge.IsDamageBlocked() && !judge.IsImpactBlocked()) {

    owner.GetField()->AddEntity(*new Hitbox(owner.GetTeam(), 0), *owner.GetTile());

    judge.AddTrigger(callback, std::ref(in), std::ref(owner));
    judge.BlockDamage();
  }
}
