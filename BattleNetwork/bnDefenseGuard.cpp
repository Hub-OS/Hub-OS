#include "bnDefenseGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnWave.h"

DefenseGuard::DefenseGuard() : DefenseRule(Priority(0))
{
}

DefenseGuard::~DefenseGuard()
{
}

const bool DefenseGuard::Check(Spell * in, Character* owner)
{
  Direction direction = Direction::NONE;

  if (in->GetDirection() == Direction::LEFT) {
    direction = Direction::RIGHT;
  }
  else if (in->GetDirection() == Direction::RIGHT) {
    direction = Direction::LEFT;
  }

  Field* field = owner->GetField();

  Spell* wave = new Wave(field, owner->GetTeam(), 8.0);
  wave->SetDirection(direction);

  field->OwnEntity(wave, owner->GetTile()->GetX(), owner->GetTile()->GetY());

  in->Delete();

  // TODO: play sound

  return true; // Guard never allows an attack to passthrough
}
