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

const bool DefenseBubbleWrap::Blocks(Spell * in, Character* owner)
{
  // weak obstacles will break like other bubbles
  owner->GetField()->AddEntity(*new Hitbox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());
  
  return false; // let anything else pass through
}
