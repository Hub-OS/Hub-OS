#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitboxSpell.h"

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

void DefenseAura::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{
  // special case: removed by wind element
  if ((attacker->GetHitboxProperties().element == Element::wind)) {
    judge.BlockDamage();

    if (callback) {
      judge.AddTrigger(callback, attacker, owner, true);
    }

    return;
  }

  // base case: impact-only hitboxes are processed further
  if ((attacker->GetHitboxProperties().flags & Hit::impact) != Hit::impact) return; // no blocking happens

  // weak obstacles will break
  auto hitbox = std::make_shared<HitboxSpell>(owner->GetTeam(), 0);
  owner->GetField()->AddEntity(hitbox, *owner->GetTile());

  judge.BlockDamage();

  if(callback) {
    judge.AddTrigger(callback, attacker, owner, false);
  }
}
