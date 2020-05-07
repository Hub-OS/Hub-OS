#include "bnDefenseVirusBody.h"

DefenseVirusBody::DefenseVirusBody() : DefenseRule(Priority(0)) {
}

DefenseVirusBody::~DefenseVirusBody() { }

Hit::Properties& DefenseVirusBody::FilterStatuses(Hit::Properties& statuses)
{
  // This is all this defense rule does is filter out flinch and recoil flags
  statuses.flags &= ~Hit::flinch;
  statuses.flags &= ~Hit::recoil;

  return statuses;
}

void DefenseVirusBody::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner) {
  // doesn't block anything
}
