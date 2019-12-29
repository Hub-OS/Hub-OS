#include "bnDefenseVirusBody.h"

DefenseVirusBody::DefenseVirusBody() : DefenseRule(Priority(0)) {
}

DefenseVirusBody::~DefenseVirusBody() { }

Hit::Properties& DefenseVirusBody::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~Hit::flinch;
  statuses.flags &= ~Hit::recoil;

  return statuses;
}
