#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Wave::Wave(Field* _field, Team _team, double speed) : Spell(_field, _team) {
  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_WAVE));
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
        auto* wave = new Wave(GetField(), GetTeam(), Wave::speed);
        wave->SetDirection(dir);
        wave->SetHitboxProperties(GetHitboxProperties());
        wave->CrackTiles(this->crackTiles);
        GetField()->AddEntity(*wave, nextTile->GetX(), nextTile->GetY());
    }
  };

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/spell_wave.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::NoEffect, [this]() { Delete(); });
  animation->AddCallback(4, spawnNext);
  animation->SetPlaybackSpeed(speed);
  animation->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);

  AUDIO.Play(AudioType::WAVE);

  HighlightTile(Battle::Tile::Highlight::solid);
}

Wave::~Wave() {
}

void Wave::OnUpdate(float _elapsed) {
  int lr = (GetDirection() == Direction::left) ? 1 : -1;
  setScale(2.f*(float)lr, 2.f);

  setPosition(GetTile()->getPosition().x, GetTile()->getPosition().y);

  GetTile()->AffectEntities(this);

  if (crackTiles) {
    GetTile()->SetState(TileState::cracked);
  }
}

bool Wave::Move(Direction _direction) {
  return false;
}

void Wave::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}

void Wave::OnDelete()
{
  Remove();
}

void Wave::CrackTiles(bool state)
{
  crackTiles = state;
}
