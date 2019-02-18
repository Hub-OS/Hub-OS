#include "bnDefenseGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnGuardHit.h"

DefenseGuard::DefenseGuard(DefenseGuard::Callback callback) : callback(callback), DefenseRule(Priority(1))
{
}

DefenseGuard::~DefenseGuard()
{
}

const bool DefenseGuard::Check(Spell * in, Character* owner)
{
  this->callback(in, owner);

  in->Delete();

  owner->GetField()->OwnEntity(new GuardHit(owner->GetField(), owner, true), owner->GetTile()->GetX(), owner->GetTile()->GetY());

  return true; // Guard never allows an attack to passthrough
}
