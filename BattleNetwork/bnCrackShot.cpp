#include "bnCrackShot.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

CrackShot::CrackShot(Field* _field, Team _team, Battle::Tile* tile) : Spell(_field, _team) {
  // Blades float over tiles 
  this->SetFloatShoe(true);

  SetLayer(-1);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_CRACKSHOT));
  setScale(2.f, 2.f);

  // TODO: how many frames does it take crackshot to move from one tile to the next?
  // For testing purposes 8 frames = 0.1 seconds

  this->SetSlideTime(sf::seconds(0.1f));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->SetPath("resources/spells/spell_panel_shot.animation");
  animation->Load();

  if (tile->GetTeam() == Team::RED) {
    animation->SetAnimation("RED_TEAM");
  }
  else if (tile->GetTeam() == Team::BLUE) {
    animation->SetAnimation("BLUE_TEAM");
  }

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);
}

CrackShot::~CrackShot() {
}

void CrackShot::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  if (GetDirection() == Direction::LEFT) {
    setScale(-2.f, 2.f);
  }
  else {
    setScale(2.f, 2.f);
  }

  // Keep moving, when we reach the end of the map, remove from play
  if (!this->IsSliding()) {
    this->SlideToTile(true);

    // Keep moving
    this->Move(this->GetDirection());

    // Move failed can only be an edge
    if (!this->GetNextTile()) {
      this->Delete();
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
    this->Delete();
  }
}

void CrackShot::OnDelete()
{
}
