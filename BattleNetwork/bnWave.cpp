#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitBox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

int Wave::numOf = 0;

Wave::Wave(Field* _field, Team _team, double speed) : Spell(_field, _team) {
  SetLayer(0);

  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_WAVE));
  this->speed = speed;

  //Components setup and load
  auto spawnNext = [this]() {
    this->EnableTileHighlight(false);
    if(this->GetTile()->GetX() > 1) {
        auto* wave = new Wave(this->GetField(), this->GetTeam(), this->speed);
        wave->SetDirection(this->GetDirection());

        this->GetField()->AddEntity(*wave, GetTile()->GetX()-1, GetTile()->GetY());
    }
  };

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);

  animation->Setup("resources/spells/spell_wave.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animate::Mode::NoEffect, [this]() { this->Delete(); });
  animation->AddCallback(4, spawnNext);
  animation->SetPlaybackSpeed(speed);
  animation->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
  props.flags |= Hit::flinch;
  props.secs = 0.2;
  this->SetHitboxProperties(props);

  AUDIO.Play(AudioType::WAVE);

  EnableTileHighlight(true);

  Logger::Log("Num of waves is: " + std::to_string(++Wave::numOf));
}

Wave::~Wave() {
  {
    Logger::Log("Wave dtor called");
    Wave::numOf = Wave::numOf - 1;
  }
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
