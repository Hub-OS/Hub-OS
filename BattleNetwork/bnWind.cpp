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

  if (!IsSliding()) {
    // Wind is active on the opposing team's area
    // Once we enter our team area, we're useless
    if (Teammate(GetTile()->GetTeam())) {
      Delete();
    }

    SlideToTile(true);
    SetSlideTime(time_cast<sf::Time>(frames(4)));
    Move(GetDirection());
  }
}

bool Wind::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Wind::Attack(Character* _entity) {
  _entity->SlideToTile(true);
  _entity->Move(GetDirection());
  _entity->FinishMove();
}

void Wind::OnDelete()
{
  Remove();
}
