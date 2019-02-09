#include "bnGear.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

Gear::Gear(Field* _field, Team _team, Direction startDir) : animation(this), Obstacle(field, team) {
  this->setTexture(LOAD_TEXTURE(MOB_METALMAN_ATLAS));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(false);
  this->SetName("MetalGear");
  this->SetTeam(_team);
  this->SetDirection(startDir);
  this->EnableTileHighlight(true);

  animation.Setup("resources/mobs/metalman/metalman.animation");
  animation.Reload();

  animation.SetAnimation("GEAR", Animate::Mode::Loop);

  this->SetHealth(999);

  animation.Update(0);

  this->slideTime = sf::seconds(2.0f); // crawl
}

Gear::~Gear() {
}

bool Gear::CanMoveTo(Battle::Tile * next)
{
  if (Entity::CanMoveTo(next)) {
    if (next->ContainsEntityType<Gear>()) {
      if (this->GetDirection() == Direction::LEFT) {
        this->SetDirection(Direction::RIGHT);
      }
      else if(this->GetDirection() == Direction::RIGHT) {
        this->SetDirection(Direction::LEFT);
      }
    }

    return true;
  }

  if (this->GetDirection() == Direction::LEFT) {
    this->SetDirection(Direction::RIGHT);
  }
  else if (this->GetDirection() == Direction::RIGHT) {
    this->SetDirection(Direction::LEFT);
  }

  return false;
}

void Gear::Update(float _elapsed) {
  animation.Update(_elapsed);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  if (!this->IsBattleActive()) return;

  // May have just finished moving
  this->tile->AffectEntities(this);

  // Keep moving
  if (!this->isSliding) {
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  if (this->GetDirection() == Direction::NONE) {
    if (this->GetPreviousDirection() == Direction::LEFT) {
      this->SetDirection(Direction::RIGHT);
    }
    else if (this->GetPreviousDirection() == Direction::RIGHT) {
      this->SetDirection(Direction::LEFT);
    }

    // Now try to move
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  Character::Update(_elapsed);
}

void Gear::OnDelete() {

}

const bool Gear::Hit(int damage, Hit::Properties props) {
  // TODO: spawn hit or something
  return true;
}

void Gear::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    isObstacle->Hit(9999);
    this->hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    isCharacter->Hit(20);
  }
}

void Gear::SetAnimation(std::string animation)
{
  this->animation.SetAnimation(animation);
}
