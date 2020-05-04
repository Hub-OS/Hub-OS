#include "bnYoYo.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

YoYo::YoYo(Field* _field, Team _team, int damage, double speed) : Spell(_field, _team) {
  // YoYo float over tiles 
  this->SetFloatShoe(true);

  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_YOYO));
  setScale(2.f, 2.f);

  this->speed = speed;

  // Adjust by speed factor
  this->SetSlideTime(sf::seconds(0.11f / (float)speed));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->SetPath("resources/spells/spell_yoyo.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::recoil;
  this->SetHitboxProperties(props);

  tileCount = hitCount = 0;
  startTile = nullptr;

  reversed = false;
}

YoYo::~YoYo() {
}

void YoYo::OnDelete() {
  if (startTile && startTile != this->GetTile()) {
    GetField()->AddEntity(*new Explosion(GetField(), GetTeam(), 1), *this->GetTile());
  }
  Remove();
}

void YoYo::OnSpawn(Battle::Tile& start) {
  startTile = &start;
}

void YoYo::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  // When moving, attack tiles normally
  // When stationary, our anim callbacks are
  // designed to hit exactly 3 times 
  // like the rhythm of the original games
  if (!this->IsSliding()) {
    if (tileCount > 0 && startTile == GetTile()) {
      this->Delete();
    }

    // Always slide
    this->SlideToTile(true);

    // Keep moving
    this->Move(this->GetDirection());

    // Retract after moving 2 spaces, if possible
    if (!this->GetNextTile() || (++tileCount == 2)) {
      if (!reversed) {
        auto direction = GetPreviousDirection();

        SetDirection(Direction::NONE);

        reversed = true;

        animation->CancelCallbacks();

        animation->AddCallback(3, [this, direction]() {
          // First, let the slide finish for this final tile...
          if (!this->IsSliding()) {
            auto hitbox = new Hitbox(GetField(), GetTeam());
            hitbox->SetHitboxProperties(GetHitboxProperties());
            GetField()->AddEntity(*hitbox, *this->GetTile());

            // After we hit 2 more times, reverse the direction 
            if (++this->hitCount == 2) {
              SetDirection(Reverse(direction));
              animation->CancelCallbacks();
            }
          }
        });
      }
    } 
  }else if (tileCount != 3) {
    // The tile counter updates when it has reached
    // center tile, when count == 2, we were spinning in place
    // So we want to skip count == 3 because it will have only
    // begun moving to the previous tile. This avoids a duplicate hit
    // caused by the animation callback when it drops 3 hitboxes.

    tile->AffectEntities(this);
  }
}

// Nothing prevents blade from cutting through
bool YoYo::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void YoYo::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT, AudioPriority::HIGHEST);
  }
}