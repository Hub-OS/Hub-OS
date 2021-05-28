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

void DefenseAura::CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner)
{
  // special case: removed by wind element
  if ((in.GetHitboxProperties().element == Element::wind)) {
    judge.BlockDamage();

    if (callback) {
      judge.AddTrigger(callback, std::ref(in), std::ref(owner), true);
    }

    return;
  }

  // base case: impact-only hitboxes are processed further
  if ((in.GetHitboxProperties().flags & Hit::impact) != Hit::impact) return; // no blocking happens

  // weak obstacles will break
  auto hitbox = new Hitbox(owner.GetTeam(), 0);
  owner.GetField()->AddEntity(*hitbox, *owner.GetTile());

  judge.BlockDamage();

  if(callback) {
    judge.AddTrigger(callback, std::ref(in), std::ref(owner), false);
  }
}
