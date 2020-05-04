#include "bnMetalBlade.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

MetalBlade::MetalBlade(Field* _field, Team _team, double speed) : Spell(_field, _team) {
  // Blades float over tiles 
  this->SetFloatShoe(true);

  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::MOB_METALMAN_ATLAS));
  setScale(2.f, 2.f);

  this->speed = speed;

  // Blades move from tile to tile in 25 frames
  // Adjust by speed factor
  this->SetSlideTime(sf::seconds(0.25f / (float)speed));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->SetPath("resources/mobs/metalman/metalman.animation");
  animation->Load();
  animation->SetAnimation("BLADE",Animator::Mode::Loop);

  auto props = Hit::DefaultProperties;
  props.damage = 40;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);
}

MetalBlade::~MetalBlade() {
}

void MetalBlade::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation->SetPlaybackSpeed(this->speed);

  // Keep moving. When we reach the end, go up or down the column, and U-turn
  if (!this->IsSliding()) {
    if(this->GetTeam() == Team::BLUE) {
        // Are we on the first column on the field?
        if (this->tile->GetX() == 1) {
         if (this->tile->GetY() == 1) {
            // If we're at the top row going left, go down
            if (this->GetDirection() == Direction::LEFT) {
              this->SetDirection(Direction::DOWN);
            }
            else {
              // Otherwise make a right
              this->SetDirection(Direction::RIGHT);
            }
          }
          else if(this->tile->GetY() == 3){
            // If were at bottom tile going left, go up
            if (this->GetDirection() == Direction::LEFT) {
              this->SetDirection(Direction::UP);
            }
            else {
              // Otherwise make a right
              this->SetDirection(Direction::RIGHT);
            }
          }
        }
    }
    else {
      // Are we on the back column on the field?
      if (this->tile->GetX() == 6) {
        if (this->tile->GetY() == 1) {
          // If we're at the top row going right, go down
          if (this->GetDirection() == Direction::RIGHT) {
            this->SetDirection(Direction::DOWN);
          }
          else {
            // Otherwise make a left
            this->SetDirection(Direction::LEFT);
          }
        }
        else if (this->tile->GetY() == 3) {
          // If were at bottom tile going right, go up
          if (this->GetDirection() == Direction::RIGHT) {
            this->SetDirection(Direction::UP);
          }
          else {
            // Otherwise make a left
            this->SetDirection(Direction::LEFT);
          }
        }
      }
    }

    // Always slide
    this->SlideToTile(true);
    
    // Keep moving
    this->Move(this->GetDirection());

    // We must have flown off screen
    if (!this->GetNextTile()) {
      this->Delete();
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
