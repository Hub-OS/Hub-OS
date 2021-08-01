#include "bnYoYo.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

YoYo::YoYo(Team _team, int damage, double speed) : Spell(_team) {
  // YoYo float over tiles
  SetFloatShoe(true);

  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::SPELL_YOYO));
  setScale(2.f, 2.f);

  YoYo::speed = speed;

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/spell_yoyo.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);

  tileCount = hitCount = 0;
  startTile = nullptr;

  reversed = false;
}

YoYo::~YoYo() {
}

void YoYo::OnDelete() {
  if (startTile && startTile != GetTile()) {
    GetField()->AddEntity(*new Explosion, *GetTile());
  }
  Remove();
}

void YoYo::OnSpawn(Battle::Tile& start) {
  startTile = &start;
}

void YoYo::OnUpdate(double _elapsed) {
  // When moving, attack tiles normally
  // When stationary, our anim callbacks are
  // designed to hit exactly 3 times
  // like the rhythm of the original games
  if (!IsSliding()) {
    if (tileCount > 0 && startTile == GetTile()) {
      Delete();
    }

    // Retract after moving 2 spaces ahead, if possible
    if (++tileCount == 3) {
      if (!reversed) {
        auto direction = GetPreviousDirection();

        SetDirection(Direction::none);

        reversed = true;

        animation->CancelCallbacks();

        animation->AddCallback(3, [this, direction]() {
          // First, let the slide finish for this final tile...
          if (!IsSliding()) {
            auto hitbox = new Hitbox(GetTeam());
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
    else {
      // Keep moving
      Slide(GetTile() + GetDirection(), frames(7), frames(0));
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
    Audio().Play(AudioType::HURT, AudioPriority::highest);
  }
}
