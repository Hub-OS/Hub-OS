#include "bnWind.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include <Swoosh/Game.h>
Wind::Wind(Field* _field, Team _team) :Spell(_field, _team) {
  SetPassthrough(true);
  SetLayer(-1);
  SetDirection(_team == Team::BLUE ? Direction::RIGHT : Direction::LEFT);
  auto props = Hit::DefaultProperties;
  props.flags = Hit::drag;
  props.drag = GetDirection();
  this->SetHitboxProperties(props);
  HighlightTile(Battle::Tile::Highlight::none);

  setTexture(LOAD_TEXTURE(SPELL_WIND));
  swoosh::game::setOrigin(*this, 0.8, 0.8);
  setScale(-2.f, 2.f);
}

Wind::~Wind() {
}

void Wind::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition() + tileOffset);

  // Strike panel and leave
  GetTile()->AffectEntities(this);

  if (!IsSliding()) {
    // Wind is active on the opposing team's area
    // Once we enter our team area, we're useless
    if (Teammate(GetTile()->GetTeam())) {
      this->Delete();
    }

    SlideToTile(true);
    Move(GetDirection());
  }
}

bool Wind::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Wind::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}