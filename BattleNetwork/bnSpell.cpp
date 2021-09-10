#include "bnSpell.h"

Spell::Spell(Team team) : Entity()
{
  SetFloatShoe(true);
  SetLayer(1);
  SetTeam(team);
}

void Spell::OnUpdate(double _elapsed) {
  if (IsTimeFrozen()) return;

  OnUpdate(_elapsed);

  setPosition(getPosition().x, getPosition().y - GetHeight());
}
