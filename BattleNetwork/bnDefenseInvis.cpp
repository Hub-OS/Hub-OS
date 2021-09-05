#include "bnDefenseInvis.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"

DefenseInvis::DefenseInvis() : DefenseRule(Priority(0), DefenseOrder::always)
{
}

DefenseInvis::~DefenseInvis()
{
}

void DefenseInvis::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)
{
  if ((in->GetHitboxProperties().flags & Hit::pierce) != Hit::pierce) {
    judge.BlockDamage();
    judge.BlockImpact();
  }
}
