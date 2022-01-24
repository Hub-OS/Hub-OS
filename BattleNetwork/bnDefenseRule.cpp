#include "bnDefenseRule.h"

DefenseRule::DefenseRule(const Priority level, const DefenseOrder& order) : 
  priorityLevel(level), order(order), replaced(false) 
{
}

DefenseRule::~DefenseRule() { }

Hit::Properties& DefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  return statuses; // pass
}

const Priority DefenseRule::GetPriorityLevel() const { return priorityLevel; }

const DefenseOrder DefenseRule::GetDefenseOrder() const
{
  return order;
}

bool DefenseRule::Added() const {
  return added;
}

void DefenseRule::OnAdd() {
  added = true;
}

const bool DefenseRule::IsReplaced() const
{
  return replaced;
}

void DefenseRule::OnReplace()
{
  // optional implementation
}
