#include "bnMetalBlade.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

MetalBlade::MetalBlade(Field* _field, Team _team, double speed) : Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::MOB_METALMAN_ATLAS);
  this->speed = speed;

  this->slideTime = sf::seconds(0.25f / (float)speed);

  animation = Animation("resources/mobs/metalman/metalman.animation");
  animation.SetAnimation("BLADE");
  animation << Animate::Mode::Loop;

  EnableTileHighlight(false);
}

MetalBlade::~MetalBlade(void) {
}

void MetalBlade::Update(float _elapsed) {
  setTexture(*texture);
  setScale(2.f, 2.f);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  animation.Update(_elapsed*(float)this->speed, this);

  // Keep moving
  if (!this->isSliding) {
    if (this->tile->GetX() == 1) {
      if (this->tile->GetY() == 2 && this->GetDirection() == Direction::LEFT) {
        this->Delete();
      }
      else if (this->tile->GetY() == 1) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::DOWN);
        }
        else {
          this->SetDirection(Direction::RIGHT);
        }
      }
      else if(this->tile->GetY() == 3){
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::UP);
        }
        else {
          this->SetDirection(Direction::RIGHT);
        }
      }
    }
    else if (this->tile->GetX() == 6) {
      this->Delete();
    }

    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool MetalBlade::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void MetalBlade::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    _entity->Hit(40);
  }
}

vector<Drawable*> MetalBlade::GetMiscComponents() {
  return vector<Drawable*>();
}