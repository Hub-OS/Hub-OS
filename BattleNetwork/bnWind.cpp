#include "bnWind.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include <Swoosh/Game.h>
Wind::Wind(Team _team) : Spell(_team) {
  SetPassthrough(true);
  SetLayer(-1);
  SetDirection(_team == Team::blue ? Direction::right : Direction::left);

  setTexture(LOAD_TEXTURE(SPELL_WIND));
  swoosh::game::setOrigin(getSprite(), 0.8, 0.8);
  setScale(-2.f, 2.f);
}

Wind::~Wind() {
}

void Wind::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition() + tileOffset);

  // Strike panel and leave
  GetTile()->AffectEntities(this);

  // Wind is active on the opposing team's area
  // Once we enter our team area, we're useless
  if (Teammate(GetTile()->GetTeam())) {
    Delete();
  }

  Slide(GetDirection(), frames(4), frames(0));
}

bool Wind::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Wind::Attack(Character* _entity) {
  _entity->Slide(GetDirection(), frames(4), frames(0));
}

void Wind::OnDelete()
{
  Remove();
}
