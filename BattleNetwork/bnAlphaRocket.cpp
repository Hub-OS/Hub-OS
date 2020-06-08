#include "bnAlphaRocket.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnHitbox.h"
#include "bnExplosion.h"
#include "bnAura.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

AlphaRocket::AlphaRocket(Field* _field, Team _team) : Obstacle(_field, _team)  {
  // AlphaRocket float over tiles 
  SetFloatShoe(true);
  SetTeam(_team);
  ShareTileSpace(true);
  SetLayer(-1);

  auto texture = Textures().GetTexture(TextureType::SPELL_ALPHA_ROCKET);
  setTexture(texture);
  setScale(2.f, 2.f);

  SetSlideTime(sf::seconds(0.5f));

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/spell_alpha_rocket.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT");
  animation->SetPlaybackMode(Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::flinch | Hit::recoil | Hit::breaking | Hit::impact;
  props.damage = 200;
  SetHitboxProperties(props);

  SetHealth(100);
}

AlphaRocket::~AlphaRocket() {
}

void AlphaRocket::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  if (GetDirection() == Direction::left) {
    setScale(2.f, 2.f);
  }
  else {
    setScale(-2.f, 2.f);
  }

  // Keep moving, when we reach the end of the map, remove from play
  if (!IsSliding()) {
    SlideToTile(true);

    // Keep moving
    Move(GetDirection());

    // Move failed can only be an edge
    if (!GetNextTile()) {
        Delete();
    }
  }

  tile->AffectEntities(this);

  if (tile->ContainsEntityType<Character>()) {
      Delete();
  }
}

// Nothing prevents AlphaRocket from moving over it
bool AlphaRocket::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void AlphaRocket::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    SetHealth(0);
    Delete();
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
    if (GetTile()->IsEdgeTile()) {
      auto additionalXTiles = {
        GetTile()->GetX() + 2
      };

      includedValid = (GetTile()->GetX() + 2 == t->GetX())
        && std::find(validYTiles.begin(), validYTiles.end(), t->GetY()) != validXTiles.end();
    }

    return valid || includedValid;
  });

  for (auto t : adj) {
    Hitbox* box = new Hitbox(GetField(), GetTeam(), 200);
    Explosion* exp = new Explosion(GetField(), GetTeam());

    box->SetHitboxProperties(GetHitboxProperties());

    GetField()->AddEntity(*box, t->GetX(), t->GetY());
    GetField()->AddEntity(*exp, t->GetX(), t->GetY());

    ENGINE.GetCamera()->ShakeCamera(10, sf::seconds(1));
  }

  Remove();
}

const float AlphaRocket::GetHeight() const
{
  return 80.0f;
}
