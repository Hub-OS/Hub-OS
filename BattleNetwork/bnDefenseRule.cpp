#include "bnDefenseRule.h"

DefenseRule::DefenseRule(Priority level) : priorityLevel(level) {
}

DefenseRule::~DefenseRule() { }

const Priority DefenseRule::GetPriorityLevel() const { return priorityLevel; }