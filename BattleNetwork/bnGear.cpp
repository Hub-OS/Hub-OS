#include "bnGear.h"
#include "bnTile.h"
#include "bnDefenseIndestructable.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

Gear::Gear(Team _team,Direction startDir) 
    : 
  startDir(startDir), 
  stopMoving(false), 
  Obstacle(team) {
  setTexture(LOAD_TEXTURE(MOB_METALMAN_ATLAS));
  setScale(2.f, 2.f);
  SetFloatShoe(false);
  SetName("MetalGear");
  SetTeam(_team);
  SetDirection(startDir);
  HighlightTile(Battle::Tile::Highlight::solid);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/mobs/metalman/metalman.animation");
  animation->Load();
  animation->SetAnimation("GEAR", Animator::Mode::Loop);

  std::function<void(BattleSceneBase&, Gear&)> battleOverCallback = [this](BattleSceneBase&, Gear&) {
      stopMoving = true;
  };

  SetHealth(999);

  animation->OnUpdate(0);

  SetSlideTime(sf::seconds(2.0f)); // crawl

  Hit::Properties props = Hit::DefaultProperties;
  props.flags |= Hit::recoil | Hit::breaking;
  SetHitboxProperties(props);

  tileStartTeam = Team::unknown;

  AddDefenseRule(new DefenseIndestructable(true));

  stopMoving = true;
}

Gear::~Gear() {
}

bool Gear::CanMoveTo(Battle::Tile * next)
{
  if (next->IsEdgeTile()) return false;

  bool valid = next ? (next->IsWalkable() && (next->GetTeam() == tileStartTeam)) : false;
  
  if (next == tile) return true;

  if (valid) {
    if (next->ContainsEntityType<Gear>()) {
      if (GetDirection() == Direction::left) {
        SetDirection(Direction::right);
      }
      else if(GetDirection() == Direction::right) {
        SetDirection(Direction::left);
      }
      return false;
    }

    return true;
  }

  if (GetDirection() == Direction::left) {
    SetDirection(Direction::right);
  }
  else if (GetDirection() == Direction::right) {
    SetDirection(Direction::left);
  }

  return false;
}

void Gear::OnUpdate(double _elapsed) {
  if (tileStartTeam == Team::unknown && tile) {
    tileStartTeam = tile->GetTeam();
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  if (stopMoving) return;

  // May have just finished moving
  tile->AffectEntities(this);

  // Keep moving
  if (!IsSliding()) {
    SlideToTile(true);
    Move(GetDirection());
  }

  if (GetDirection() == Direction::none) {
    if (GetPreviousDirection() == Direction::left) {
      SetDirection(Direction::right);
    }
    else if (GetPreviousDirection() == Direction::right) {
      SetDirection(Direction::left);
    }
    else if (GetPreviousDirection() == Direction::none) {
      SetDirection(startDir); // Todo: should slide mechanism remove previous direction info? This works but not necessary
    }

    // Now try to move
    SlideToTile(true);
    Move(GetDirection());
  }

}

void Gear::OnDelete() {
    Remove();
}

void Gear::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    auto props = Hit::DefaultProperties;
    props.damage = 9999;
    isObstacle->Hit(props);
    hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    auto props = Hit::DefaultProperties;
    props.damage = 20;
    isCharacter->Hit(props);
  }
}

void Gear::OnBattleStart()
{
    Logger::Log("Gear::OnBattleStart()");
    stopMoving = false;
}

void Gear::OnBattleStop()
{
    Logger::Log("Gear::OnBattleStop()");
    stopMoving = true;
}
