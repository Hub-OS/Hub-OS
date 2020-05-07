#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"
#include "bnHitbox.h"

DefenseAura::DefenseAura(const DefenseAura::Callback& callback) : DefenseRule(Priority(4))
{
  this->callback = callback;
}

DefenseAura::DefenseAura() : DefenseRule(Priority(4)) {
  callback = nullptr;
}

DefenseAura::~DefenseAura()
{
}

void DefenseAura::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  if ((in.GetHitboxProperties().flags & Hit::impact) != Hit::impact) return;

  arbiter.BlockDamage();

  if(callback) { 
    arbiter.AddTrigger(callback, std::ref(in), std::ref(owner)); 
  }
}
