#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Wave::Wave(Field* _field, Team _team, double speed) : Spell(_field, _team) {
  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_WAVE));
  this->speed = speed;

  //Components setup and load
  auto spawnNext = [this]() {
    this->HighlightTile(Battle::Tile::Highlight::none);

    Battle::Tile* nextTile = nullptr;
    Direction dir = Direction::NONE;

    if (this->GetTeam() == Team::BLUE) {
      nextTile = this->GetField()->GetAt(GetTile()->GetX() - 1, GetTile()->GetY());
      dir = Direction::LEFT;
    } 
    else {
      nextTile = this->GetField()->GetAt(GetTile()->GetX() + 1, GetTile()->GetY());
      dir = Direction::RIGHT;
    }

    if(nextTile && nextTile->IsWalkable() && !nextTile->IsEdgeTile()) {
        auto* wave = new Wave(this->GetField(), this->GetTeam(), this->speed);
        wave->SetDirection(dir);

        this->GetField()->AddEntity(*wave, nextTile->GetX(), nextTile->GetY());
    }
  };

  animation = CreateComponent<AnimationComponent>(this);
  this->RegisterComponent(animation);

  animation->SetPath("resources/spells/spell_wave.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::NoEffect, [this]() { this->Delete(); });
  animation->AddCallback(4, spawnNext);
  animation->SetPlaybackSpeed(speed);
  animation->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);

  AUDIO.Play(AudioType::WAVE);

  this->HighlightTile(Battle::Tile::Highlight::solid);
}

Wave::~Wave() {
}

void Wave::OnUpdate(float _elapsed) {
  int lr = (this->GetDirection() == Direction::LEFT) ? 1 : -1;
  setScale(2.f*(float)lr, 2.f);

  setPosition(GetTile()->getPosition().x, GetTile()->getPosition().y);

  if (!this->IsDeleted()) {
    GetTile()->AffectEntities(this);
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
