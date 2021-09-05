#include "bnDefenseBubbleWrap.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseBubbleWrap::DefenseBubbleWrap() : popped(false), DefenseRule(Priority(0), DefenseOrder::always)
{
}

DefenseBubbleWrap::~DefenseBubbleWrap()
{
}

const bool DefenseBubbleWrap::IsPopped() const
{
  return popped;
}

Hit::Properties& DefenseBubbleWrap::FilterStatuses(Hit::Properties& statuses) {
  if(statuses.element == Element::elec)
    statuses.damage *= 2;
  return statuses;
}

void DefenseBubbleWrap::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)
{
  if ((in->GetHitboxProperties().flags & Hit::impact) == 0) return;

  // weak obstacles will break like other bubbles
  auto hitbox = std::make_shared<Hitbox>(owner->GetTeam(), 0);
  owner->GetField()->AddEntity(hitbox, *owner->GetTile());

  auto props = in->GetHitboxProperties();
  if ((props.flags & Hit::impact) == Hit::impact) {

    if (in->GetElement() != Element::elec) {
      judge.BlockDamage();
    }
    else {
      judge.SignalDefenseWasPierced();
    }

    popped = true;
  }
}
