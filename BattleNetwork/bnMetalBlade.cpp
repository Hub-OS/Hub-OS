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
  props.flags |= Hit::flash;
  SetHitboxProperties(props);
}

MetalBlade::~MetalBlade() {
}

void MetalBlade::OnUpdate(double _elapsed) {
  animation->SetPlaybackSpeed(speed);

  // Keep moving. When we reach the end, go up or down the column, and U-turn
  if (!IsSliding()) {
    if(GetTeam() == Team::blue) {
        // Are we on the first column on the field?
        if (tile->GetX() == 1) {
         if (tile->GetY() == 1) {
            // If we're at the top row going left, go down
            if (GetMoveDirection() == Direction::left) {
              SetMoveDirection(Direction::down);
            }
            else {
              // Otherwise make a right
              SetMoveDirection(Direction::right);
            }
          }
          else if(tile->GetY() == 3){
            // If were at bottom tile going left, go up
            if (GetMoveDirection() == Direction::left) {
              SetMoveDirection(Direction::up);
            }
            else {
              // Otherwise make a right
              SetMoveDirection(Direction::right);
            }
          }
        }
    }
    else {
      // Are we on the back column on the field?
      if (tile->GetX() == 6) {
        if (tile->GetY() == 1) {
          // If we're at the top row going right, go down
          if (GetMoveDirection() == Direction::right) {
            SetMoveDirection(Direction::down);
          }
          else {
            // Otherwise make a left
            SetMoveDirection(Direction::left);
          }
        }
        else if (tile->GetY() == 3) {
          // If were at bottom tile going right, go up
          if (GetMoveDirection() == Direction::right) {
            SetMoveDirection(Direction::up);
          }
          else {
            // Otherwise make a left
            SetMoveDirection(Direction::left);
          }
        }
      }
    }

    // Always slide
    if (!IsSliding()) {
      if (!CanMoveTo(GetTile() + GetMoveDirection())) {
        Delete();
      }
      else {
        int ispeed = static_cast<unsigned>(25 * speed);
        Slide(GetTile() + GetMoveDirection(), frames(ispeed), frames(0));
      }
    }
  }

  tile->AffectEntities(this);
}

// Nothing prevents blade from cutting through
bool MetalBlade::CanMoveTo(Battle::Tile* tile) {
  return !tile->IsEdgeTile();
}

void MetalBlade::Attack(Character* _entity) {
   _entity->Hit(GetHitboxProperties());
}

void MetalBlade::OnDelete()
{
  Remove();
}
