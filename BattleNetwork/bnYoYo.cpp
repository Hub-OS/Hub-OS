#include "bnYoYo.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

YoYo::YoYo(Field* _field, Team _team, int damage, double speed) : Spell(_field, _team) {
  // YoYo float over tiles
  SetFloatShoe(true);

  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_YOYO));
  setScale(2.f, 2.f);

  YoYo::speed = speed;

  // Adjust by speed factor
  SetSlideTime(sf::seconds(0.11f / (float)speed));

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/spell_yoyo.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::recoil;
  SetHitboxProperties(props);

  tileCount = hitCount = 0;
  startTile = nullptr;

  reversed = false;
}

YoYo::~YoYo() {
}

void YoYo::OnDelete() {
  if (startTile && startTile != GetTile()) {
    GetField()->AddEntity(*new Explosion(GetField(), GetTeam(), 1), *GetTile());
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
  if (!IsSliding()) {
    if (tileCount > 0 && startTile == GetTile()) {
      Delete();
    }

    // Always slide
    SlideToTile(true);

    // Keep moving
    Move(GetDirection());

    // Retract after moving 2 spaces, if possible
    if (!GetNextTile() || (++tileCount == 2)) {
      if (!reversed) {
        auto direction = GetPreviousDirection();

        SetDirection(Direction::none);

        reversed = true;

        animation->CancelCallbacks();

        animation->AddCallback(3, [this, direction]() {
          // First, let the slide finish for this final tile...
          if (!IsSliding()) {
            auto hitbox = new Hitbox(GetField(), GetTeam());
            hitbox->SetHitboxProperties(GetHitboxProperties());
            GetField()->AddEntity(*hitbox, *GetTile());

            // After we hit 2 more times, reverse the direction
            if (++hitCount == 2) {
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
    AUDIO.Play(AudioType::HURT, AudioPriority::highest);
  }
}
