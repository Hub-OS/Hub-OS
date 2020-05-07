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
  if(statuses.element == Element::elec)
    statuses.damage *= 2;
  return statuses;
}

void DefenseBubbleWrap::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  // weak obstacles will break like other bubbles
  owner.GetField()->AddEntity(*new Hitbox(owner.GetField(), owner.GetTeam(), 0), owner.GetTile()->GetX(), owner.GetTile()->GetY());

  auto props = in.GetHitboxProperties();
  if ((props.flags & Hit::impact) == Hit::impact) {

    if (in.GetElement() == Element::elec) {
      arbiter.BlockDamage();
    }
  }
}
