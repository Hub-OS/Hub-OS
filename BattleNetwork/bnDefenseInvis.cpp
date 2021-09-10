#include "bnDefenseInvis.h"
#include "bnEntity.h"
#include "bnField.h"

DefenseInvis::DefenseInvis() : DefenseRule(Priority(0), DefenseOrder::always)
{
}

DefenseInvis::~DefenseInvis()
{
}

void DefenseInvis::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{
  if ((attacker->GetHitboxProperties().flags & Hit::pierce) != Hit::pierce) {
    judge.BlockDamage();
    judge.BlockImpact();
  }
}
