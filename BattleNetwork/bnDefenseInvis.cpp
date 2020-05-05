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

const bool DefenseInvis::CanBlock(DefenseResolutionArbiter& arbiter, Spell& in, Character& owner)
{
  return (in.GetHitboxProperties().flags & Hit::pierce) != Hit::pierce;
}
