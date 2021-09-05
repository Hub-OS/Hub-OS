#include "bnWind.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include <Swoosh/Game.h>
Wind::Wind(Team _team) : Spell(_team) {
  SetPassthrough(true);
  SetLayer(-1);

  setTexture(LOAD_TEXTURE(SPELL_WIND));
  swoosh::game::setOrigin(getSprite(), 0.8, 0.8);
  setScale(-2.f, 2.f);

  SetHitboxProperties({
    0,
    Hit::drag,
    Element::wind,
    0,
    { GetDirection(), 9 },
  });
}

Wind::~Wind() {
}

void Wind::OnUpdate(double _elapsed) {
  // Wind is active on the opposing team's area
  // Once we enter our team area, we're useless
  bool opposingTeamOnly = deleteOnTeam && Teammate(GetTile()->GetTeam());

  // Unbiased wind hitboxes delete when reaching the end of the field...
  bool reachedEnd = !IsSliding() && GetTile()->IsEdgeTile();

  // Test tile or apply hitbox...
  if (opposingTeamOnly || reachedEnd) {
    Delete();
  }
  else {
    // Strike panel and leave
    GetTile()->AffectEntities(*this);
  }

  Slide(GetTile() + GetDirection(), frames(4), frames(0));

  SetHitboxProperties({
    0,
    Hit::drag,
    Element::wind,
    0,
    { GetDirection(), 9 }
  });
}

bool Wind::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Wind::DeleteOnTeamTile()
{
  deleteOnTeam = true;
}

void Wind::Attack(std::shared_ptr<Character> _entity) {
  _entity->Hit(GetHitboxProperties());
}

void Wind::OnDelete()
{
  Remove();
}
