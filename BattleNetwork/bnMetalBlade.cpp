#include "bnMetalBlade.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

MetalBlade::MetalBlade(Team _team, double speed) : Spell(_team) {
  // Blades float over tiles 
  SetFloatShoe(true);

  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::MOB_METALMAN_ATLAS));
  setScale(2.f, 2.f);

  MetalBlade::speed = speed;

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/mobs/metalman/metalman.animation");
  animation->Load();
  animation->SetAnimation("BLADE",Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = 40;
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);
}

MetalBlade::~MetalBlade() {
}

void MetalBlade::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation->SetPlaybackSpeed(speed);

  // Keep moving. When we reach the end, go up or down the column, and U-turn
  if (!IsSliding()) {
    if(GetTeam() == Team::blue) {
        // Are we on the first column on the field?
        if (tile->GetX() == 1) {
         if (tile->GetY() == 1) {
            // If we're at the top row going left, go down
            if (GetDirection() == Direction::left) {
              SetDirection(Direction::down);
            }
            else {
              // Otherwise make a right
              SetDirection(Direction::right);
            }
          }
          else if(tile->GetY() == 3){
            // If were at bottom tile going left, go up
            if (GetDirection() == Direction::left) {
              SetDirection(Direction::up);
            }
            else {
              // Otherwise make a right
              SetDirection(Direction::right);
            }
          }
        }
    }
    else {
      // Are we on the back column on the field?
      if (tile->GetX() == 6) {
        if (tile->GetY() == 1) {
          // If we're at the top row going right, go down
          if (GetDirection() == Direction::right) {
            SetDirection(Direction::down);
          }
          else {
            // Otherwise make a left
            SetDirection(Direction::left);
          }
        }
        else if (tile->GetY() == 3) {
          // If were at bottom tile going right, go up
          if (GetDirection() == Direction::right) {
            SetDirection(Direction::up);
          }
          else {
            // Otherwise make a left
            SetDirection(Direction::left);
          }
        }
      }
    }

    // Always slide
    if (!IsSliding()) {
      if (!CanMoveTo(GetTile() + GetDirection())) {
        Delete();
      }
      else {
        Slide(GetDirection(), frames(static_cast<unsigned>(25*speed)));
      }
    }
  }

  tile->AffectEntities(this);
}

// Nothing prevents blade from cutting through
bool MetalBlade::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void MetalBlade::Attack(Character* _entity) {
   _entity->Hit(GetHitboxProperties());
}

void MetalBlade::OnDelete()
{
  Remove();
}
