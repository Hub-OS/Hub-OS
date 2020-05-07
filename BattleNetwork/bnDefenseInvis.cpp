#include "bnDefenseInvis.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"

DefenseInvis::DefenseInvis() : DefenseRule(0)
{
}

DefenseInvis::~DefenseInvis()
{
}

void DefenseInvis::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner)
{
  if ((in.GetHitboxProperties().flags & Hit::pierce) != Hit::pierce) {
    arbiter.BlockDamage();
    arbiter.BlockImpact();
  }
}
