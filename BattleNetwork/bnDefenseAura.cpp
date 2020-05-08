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
  if ((in.GetHitboxProperties().flags & Hit::impact) != Hit::impact) return;

  judge.BlockDamage();

  if(callback) { 
    judge.AddTrigger(callback, std::ref(in), std::ref(owner)); 
  }
}
