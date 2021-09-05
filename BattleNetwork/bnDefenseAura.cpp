#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"
#include "bnHitbox.h"

DefenseAura::DefenseAura(const DefenseAura::Callback& callback) : DefenseRule(Priority(4), DefenseOrder::always)
{
  this->callback = callback;
}

DefenseAura::DefenseAura() : DefenseRule(Priority(4), DefenseOrder::always) {
  callback = nullptr;
}

DefenseAura::~DefenseAura()
{
}

void DefenseAura::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)
{
  // special case: removed by wind element
  if ((in->GetHitboxProperties().element == Element::wind)) {
    judge.BlockDamage();

    if (callback) {
      judge.AddTrigger(callback, in, owner, true);
    }

    return;
  }

  // base case: impact-only hitboxes are processed further
  if ((in->GetHitboxProperties().flags & Hit::impact) != Hit::impact) return; // no blocking happens

  // weak obstacles will break
  auto hitbox = std::make_shared<Hitbox>(owner->GetTeam(), 0);
  owner->GetField()->AddEntity(hitbox, *owner->GetTile());

  judge.BlockDamage();

  if(callback) {
    judge.AddTrigger(callback, in, owner, false);
  }
}
