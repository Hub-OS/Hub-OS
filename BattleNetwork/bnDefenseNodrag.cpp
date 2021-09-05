#include "bnDefenseNodrag.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"

DefenseNodrag::DefenseNodrag() : DefenseRule(Priority(2), DefenseOrder::always)
{
}

DefenseNodrag::~DefenseNodrag()
{
}

Hit::Properties& DefenseNodrag::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~(Hit::drag);
  return statuses;
}

void DefenseNodrag::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)
{
  // does nothing
}
