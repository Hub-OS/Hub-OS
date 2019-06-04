#include "bnMetalBlade.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

MetalBlade::MetalBlade(Field* _field, Team _team, double speed) : Spell() {
  // Blades float over tiles 
  this->SetFloatShoe(true);

  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  auto texture = TEXTURES.GetTexture(TextureType::MOB_METALMAN_ATLAS);
  setTexture(*texture);
  setScale(2.f, 2.f);

  this->speed = speed;

  // Blades move from tile to tile in 25 frames
  // Adjust by speed factor
  this->slideTime = sf::seconds(0.25f / (float)speed);

  animation = Animation("resources/mobs/metalman/metalman.animation");
  animation.SetAnimation("BLADE");
  animation << Animate::Mode::Loop;

  auto props = Hit::DefaultProperties;
  props.damage = 40;
  this->SetHitboxProperties(props);

  EnableTileHighlight(false);
}

MetalBlade::~MetalBlade() {
}

void MetalBlade::Update(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  // Animate based on speed factor
  animation.Update(_elapsed*(float)this->speed, *this);

  // Keep moving. When we reach the end, go up or down the column, and U-turn
  if (!this->isSliding) {
    if(this->GetTeam() == Team::BLUE) {
        // Are we on the first column on the field?
        if (this->tile->GetX() == 1) {
          // Middle row just deletes at the end
          if (this->tile->GetY() == 2 && this->GetDirection() == Direction::LEFT) {
            this->Delete();
          }
          else if (this->tile->GetY() == 1) {
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
        else if (this->tile->GetX() == 6) {
          // If we're back on our team's side, delete
          this->Delete();
        }
    } else {
        // Are we on the back column on the field?
        if (this->tile->GetX() == 6) {
          // Middle row just deletes at the end
          if (this->tile->GetY() == 2 && this->GetDirection() == Direction::RIGHT) {
            this->Delete();
          }
          else if (this->tile->GetY() == 1) {
            // If we're at the top row going right, go down
            if (this->GetDirection() == Direction::RIGHT) {
              this->SetDirection(Direction::DOWN);
            }
            else {
              // Otherwise make a left
              this->SetDirection(Direction::LEFT);
            }
          }
          else if(this->tile->GetY() == 3){
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
        else if (this->tile->GetX() == 1) {
          // If we're back on our team's side, delete
          this->Delete();
        }        
    }

    // Always slide
    this->SlideToTile(true);
    
    // Keep moving
    this->Move(this->GetDirection());
  }

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

// Nothing prevents blade from cutting through
bool MetalBlade::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void MetalBlade::Attack(Character* _entity) {
   _entity->Hit(GetHitboxProperties());
}