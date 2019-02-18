#include "bnDefenseAura.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"

DefenseAura::DefenseAura() : DefenseRule(Priority(100))
{
}

DefenseAura::~DefenseAura()
{
}

const bool DefenseAura::Check(Spell * in, Character* owner)
{
  // TODO: if in == Wind

  return true; // barrier never lets attacks passthrough
}
