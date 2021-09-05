#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Wave::Wave(Team _team, double speed, int damage) : Spell(_team) {
  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::SPELL_WAVE));
  Wave::speed = speed;

  //Components setup and load
  auto spawnNext = [this]() {
    HighlightTile(Battle::Tile::Highlight::none);

    Battle::Tile* nextTile = nullptr;
    Direction dir = Direction::none;

    if (GetTeam() == Team::blue) {
      nextTile = GetField()->GetAt(GetTile()->GetX() - 1, GetTile()->GetY());
      dir = Direction::left;
    }
    else {
      nextTile = GetField()->GetAt(GetTile()->GetX() + 1, GetTile()->GetY());
      dir = Direction::right;
    }

    if(nextTile && nextTile->IsWalkable() && !nextTile->IsEdgeTile()) {
      auto wave = std::make_shared<Wave>(GetTeam(), Wave::speed);
      wave->SetDirection(dir);
      wave->SetHitboxProperties(GetHitboxProperties());
      wave->CrackTiles(this->crackTiles);
      wave->PoisonTiles(this->poisonTiles);
      GetField()->AddEntity(wave, nextTile->GetX(), nextTile->GetY());
    }
  };

  animation = CreateComponent<AnimationComponent>(weak_from_this());
  animation->SetPath("resources/spells/spell_wave.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::NoEffect, [this]() { Delete(); });
  animation->AddCallback(4, spawnNext);
  animation->SetPlaybackSpeed(speed);
  animation->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flash;
  SetHitboxProperties(props);

  Audio().Play(AudioType::WAVE);

  HighlightTile(Battle::Tile::Highlight::solid);
}

Wave::~Wave() {
}

void Wave::OnUpdate(double _elapsed) {
  int lr = (GetDirection() == Direction::left) ? 1 : -1;
  setScale(2.f*(float)lr, 2.f);

  auto tile = GetTile();
  tile->AffectEntities(*this);

  if (tile->IsWalkable()) {
    if (crackTiles) {
      tile->SetState(TileState::cracked);
    }

    if (poisonTiles) {
      tile->SetState(TileState::poison);
    }
  }
}

void Wave::Attack(std::shared_ptr<Character> _entity) {
  _entity->Hit(GetHitboxProperties());
}

void Wave::OnDelete()
{
  Remove();
}

void Wave::PoisonTiles(bool state)
{
  poisonTiles = state;
}

void Wave::CrackTiles(bool state)
{
  crackTiles = state;
}
