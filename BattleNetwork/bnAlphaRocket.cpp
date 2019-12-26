#include "bnAlphaRocket.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnHitBox.h"
#include "bnExplosion.h"
#include "bnAura.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

AlphaRocket::AlphaRocket(Field* _field, Team _team) : Obstacle(_field, _team)  {
  // AlphaRocket float over tiles 
  this->SetFloatShoe(true);
  this->SetTeam(_team);
  this->ShareTileSpace(true);
  SetLayer(-1);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_ALPHA_ROCKET);
  setTexture(*texture);
  setScale(2.f, 2.f);

  this->SetSlideTime(sf::seconds(0.5f));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->Setup("resources/spells/spell_alpha_rocket.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT");
  animation->SetPlaybackMode(Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::flinch | Hit::recoil | Hit::breaking | Hit::impact;
  props.damage = 200;
  this->SetHitboxProperties(props);

  SetHealth(100);

  Logger::Log("rocket spawned");
}

AlphaRocket::~AlphaRocket() {
}

void AlphaRocket::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  if (GetDirection() == Direction::LEFT) {
    setScale(2.f, 2.f);
  }
  else {
    setScale(-2.f, 2.f);
  }

  // Keep moving, when we reach the end of the map, remove from play
  if (!this->IsSliding()) {
    this->SlideToTile(true);

    // Keep moving
    this->Move(this->GetDirection());

    // Move failed can only be an edge
    if (!this->GetNextTile()) {
      this->SetHealth(0);
      this->TryDelete();
    }
  }

  tile->AffectEntities(this);

  if (tile->ContainsEntityType<Character>()) {
    this->TryDelete();
  }
}

// Nothing prevents AlphaRocket from moving over it
bool AlphaRocket::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void AlphaRocket::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    this->SetHealth(0);
    this->TryDelete();
  }
}

const bool AlphaRocket::OnHit(const Hit::Properties props)
{
  return true;
}

void AlphaRocket::OnDelete()
{
  auto adj = field->FindTiles([this](Battle::Tile *t) -> bool {
    if (t->IsEdgeTile()) return false;

    auto validXTiles = {
      GetTile()->GetX(), GetTile()->GetX() - 1, GetTile()->GetX() + 1
    };

    auto validYTiles = {
      GetTile()->GetY(), GetTile()->GetY() - 1, GetTile()->GetY() + 1
    };

    bool valid = std::find(validXTiles.begin(), validXTiles.end(), t->GetX()) != validXTiles.end()
      && std::find(validYTiles.begin(), validYTiles.end(), t->GetY()) != validXTiles.end();
    
    bool includedValid = false;

    // Missile hit the back row, include the right-most column
    if (this->GetTile()->IsEdgeTile()) {
      auto additionalXTiles = {
        GetTile()->GetX() + 2
      };

      includedValid = (GetTile()->GetX() + 2 == t->GetX())
        && std::find(validYTiles.begin(), validYTiles.end(), t->GetY()) != validXTiles.end();
    }

    return valid || includedValid;
  });

  for (auto t : adj) {
    HitBox* box = new HitBox(GetField(), GetTeam(), 200);
    Explosion* exp = new Explosion(GetField(), GetTeam());

    box->SetHitboxProperties(this->GetHitboxProperties());

    GetField()->AddEntity(*box, t->GetX(), t->GetY());
    GetField()->AddEntity(*exp, t->GetX(), t->GetY());

    ENGINE.GetCamera()->ShakeCamera(10, sf::seconds(1));
  }

  this->Delete();
}

const float AlphaRocket::GetHeight() const
{
  return 80.0f;
}
