#include "bnYoYo.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnHitBox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

YoYo::YoYo(Field* _field, Team _team, int damage, double speed) : Spell(_field, _team) {
  // YoYo float over tiles 
  this->SetFloatShoe(true);

  SetLayer(0);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_YOYO);
  setTexture(*texture);
  setScale(2.f, 2.f);

  this->speed = speed;

  // Adjust by speed factor
  this->SetSlideTime(sf::seconds(0.13f / (float)speed));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->Setup("resources/spells/spell_yoyo.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::recoil;
  this->SetHitboxProperties(props);

  EnableTileHighlight(false);

  tileCount = hitCount = 0;
  startTile = nullptr;

  reversed = false;
}

YoYo::~YoYo() {
}

void YoYo::OnDelete() {
  if (startTile && startTile != this->GetTile()) {
    GetField()->AddEntity(*new Explosion(GetField(), GetTeam(), 1), GetTile()->GetX(), GetTile()->GetY());
  }
}

void YoYo::OnUpdate(float _elapsed) {
  // TODO: another good case for OnSpawn() or something
  if (!startTile) {
    startTile = GetTile();
  }

  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation->SetPlaybackSpeed(this->speed);

  // Keep moving. When we reach the end, go up or down the column, and U-turn
  if (!this->IsSliding()) {
    if (tileCount > 0 && startTile == GetTile()) {
      this->Delete();
    }

    // Always slide
    this->SlideToTile(true);

    // Keep moving
    this->Move(this->GetDirection());

    // Retract after moving 2 spaces
    if (!this->GetNextTile() || ++tileCount == 2) {
      if (!reversed) {
        auto direction = GetDirection();

        SetDirection(Direction::NONE);
        reversed = true;

        animation->AddCallback(2, [this, direction]() {
          auto hitbox = new HitBox(GetField(), GetTeam());
          hitbox->SetHitboxProperties(GetHitboxProperties());
          GetField()->AddEntity(*hitbox, GetTile()->GetX(), GetTile()->GetY());

          if (++this->hitCount == 3) {
            SetDirection(Reverse(direction));
            animation->CancelCallbacks();
          }
        });
      }
    }
  }

  tile->AffectEntities(this);
}

// Nothing prevents blade from cutting through
bool YoYo::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void YoYo::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT);
  }
}