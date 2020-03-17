#include "bnFishy.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Fishy::Fishy(Field* _field, Team _team, double speed) : Obstacle(field, team) {
  SetLayer(0);
  field = _field;
  hit = false;
  
  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/fishy_temp.png"));
  setScale(2.f, 2.f);
  // why do we need to do this??
  // The super constructor is failing to set this value
  // TODO: find out why
  this->team = _team;

  this->speed = speed;

  this->SetSlideTime(sf::seconds(0.1f));
  this->SetHealth(1);

  AUDIO.Play(AudioType::TOSS_ITEM_LITE, AudioPriority::LOWEST);

  Hit::Properties props;
  props.damage = 80;
  props.flags |= Hit::recoil | Hit::flinch;
  this->SetHitboxProperties(props);

  this->SetFloatShoe(true);
}

Fishy::~Fishy() {
}

void Fishy::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x - 40.0f, tile->getPosition().y + tileOffset.y - 120.0f);

  if (this->GetTile()->GetX() == 6) {
    this->Delete();
  }

  // Keep moving
  if (!this->IsSliding()) {
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  tile->AffectEntities(this);
}

bool Fishy::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Fishy::OnHit(const Hit::Properties props) {
  return true; // fishy blocks everything
}

void Fishy::Attack(Character* _entity) {
  std::cout << "fishy team: " << (int)this->GetTeam() << std::endl;

  if (!hit) {

    hit = _entity->Hit(this->GetHitboxProperties());
    if (hit) {
      // end this
      this->Delete();
    }
  }
}