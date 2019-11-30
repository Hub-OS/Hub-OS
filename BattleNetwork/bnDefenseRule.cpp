#include "bnDefenseRule.h"

DefenseRule::DefenseRule(Priority level) : priorityLevel(level), replaced(false) {
}

DefenseRule::~DefenseRule() { }

Hit::Properties& DefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  return statuses; // pass
}

const Priority DefenseRule::GetPriorityLevel() const { return priorityLevel; }

const bool DefenseRule::IsReplaced() const
{
  return replaced;
}
