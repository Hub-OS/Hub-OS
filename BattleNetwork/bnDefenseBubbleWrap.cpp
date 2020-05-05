#include "bnDefenseBubbleWrap.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseBubbleWrap::DefenseBubbleWrap() : DefenseRule(Priority(0))
{
}

DefenseBubbleWrap::~DefenseBubbleWrap()
{
}

Hit::Properties& DefenseBubbleWrap::FilterStatuses(Hit::Properties& statuses) {
  if(statuses.element == Element::ELEC)
    statuses.damage *= 2;
  return statuses;
}

const bool DefenseBubbleWrap::CanBlock(DefenseResolutionArbiter& arbiter, Spell& in, Character& owner)
{
  // weak obstacles will break like other bubbles
  owner.GetField()->AddEntity(*new Hitbox(owner.GetField(), owner.GetTeam(), 0), owner.GetTile()->GetX(), owner.GetTile()->GetY());

  auto props = in.GetHitboxProperties();
  if ((props.flags & Hit::impact) == Hit::impact) {

    if (in.GetElement() == Element::ELEC) {
      arbiter.BlockDamage();
      return true;
    }
    else {
      // TODO: trigger to remove bubble wrap
    }
  }

  return false; // let anything else pass through
}
