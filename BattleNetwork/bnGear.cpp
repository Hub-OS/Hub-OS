#include "bnGear.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

Gear::Gear(Field* _field, Team _team, Direction startDir) : startDir(startDir), Obstacle(field, team) {
  this->setTexture(LOAD_TEXTURE(MOB_METALMAN_ATLAS));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(false);
  this->SetName("MetalGear");
  this->SetTeam(_team);
  this->SetDirection(startDir);
  this->EnableTileHighlight(true);

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->Setup("resources/mobs/metalman/metalman.animation");
  animation->Reload();

  animation->SetAnimation("GEAR", Animate::Mode::Loop);

  this->SetHealth(999);

  animation->OnUpdate(0);

  this->SetSlideTime(sf::seconds(2.0f)); // crawl

  Hit::Properties props = Hit::DefaultProperties;
  props.flags |= Hit::recoil | Hit::breaking;
  this->SetHitboxProperties(props);

  tileStartTeam = Team::UNKNOWN;
}

Gear::~Gear() {
}

bool Gear::CanMoveTo(Battle::Tile * next)
{
  bool valid = next ? (next->IsWalkable() && (next->GetTeam() == tileStartTeam)) : false;
  
  if (next == tile) return true;

  if (valid) {
    if (next->ContainsEntityType<Gear>()) {
      if (this->GetDirection() == Direction::LEFT) {
        this->SetDirection(Direction::RIGHT);
      }
      else if(this->GetDirection() == Direction::RIGHT) {
        this->SetDirection(Direction::LEFT);
      }
      return false;
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

void Gear::OnUpdate(float _elapsed) {
  if (tileStartTeam == Team::UNKNOWN && tile) {
    tileStartTeam = tile->GetTeam();
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  if (!this->IsBattleActive()) return;

  // May have just finished moving
  this->tile->AffectEntities(this);

  // Keep moving
  if (!this->IsSliding()) {
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
    else if (this->GetPreviousDirection() == Direction::NONE) {
      this->SetDirection(startDir); // Todo: should slide mechanism remove previous direction info? This works but not necessary
    }

    // Now try to move
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

}

void Gear::OnDelete() {

}

const bool Gear::OnHit(const Hit::Properties props) {
  return true;
}

void Gear::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    auto props = Hit::DefaultProperties;
    props.damage = 9999;
    isObstacle->Hit(props);
    this->hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    auto props = Hit::DefaultProperties;
    props.damage = 20;
    isCharacter->Hit(props);
  }
}