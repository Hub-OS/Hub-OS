#include "bnCrackShot.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

CrackShot::CrackShot(Team _team,Battle::Tile* tile) : Spell(_team) {
  // Blades float over tiles 
  SetFloatShoe(true);

  SetLayer(-1);

  setTexture(Textures().GetTexture(TextureType::SPELL_CRACKSHOT));
  setScale(2.f, 2.f);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/spell_panel_shot.animation");
  animation->Load();

  if (tile->GetTeam() == Team::red) {
    animation->SetAnimation("RED_TEAM");
  }
  else if (tile->GetTeam() == Team::blue) {
    animation->SetAnimation("BLUE_TEAM");
  }

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::flash;
  SetHitboxProperties(props);
}

CrackShot::~CrackShot() {
}

void CrackShot::OnUpdate(double _elapsed) {
  if (GetMoveDirection() == Direction::left) {
    setScale(-2.f, 2.f);
  }
  else {
    setScale(2.f, 2.f);
  }

  // Keep moving, when we reach the end of the map, remove from play
  if (!IsSliding()) {
    if (!CanMoveTo(GetTile() + GetMoveDirection())) {
      Delete();
    }
    else {
      Slide(GetTile() + GetMoveDirection(), frames(8), frames(0));
    }
  }

  tile->AffectEntities(this);
}

// Nothing prevents blade from cutting through
bool CrackShot::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void CrackShot::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    Delete();
  }
}

void CrackShot::OnDelete()
{
  Remove();
}
