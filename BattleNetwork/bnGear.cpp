#include "bnGear.h"
#include "bnTile.h"
#include "bnDefenseIndestructable.h"
#include "bnDefenseNodrag.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

Gear::Gear(Team _team,Direction startDir) : 
  startDir(startDir), 
  stopMoving(false), 
  Obstacle(team) {
  setTexture(LOAD_TEXTURE(MOB_METALMAN_ATLAS));
  setScale(2.f, 2.f);
  SetFloatShoe(false);
  SlidesOnTiles(false);
  SetName("MetalGear");
  SetTeam(_team);
  SetMoveDirection(startDir);
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

  Hit::Properties props = Hit::DefaultProperties;
  props.flags |= Hit::flinch | Hit::breaking;
  SetHitboxProperties(props);

  tileStartTeam = Team::unknown;

  AddDefenseRule((indestructable = new DefenseIndestructable(true)));
  AddDefenseRule((nodrag = new DefenseNodrag()));

  stopMoving = true;
}

Gear::~Gear() {
  RemoveDefenseRule(indestructable);
  RemoveDefenseRule(nodrag);
  delete indestructable;
  delete nodrag;
}

bool Gear::CanMoveTo(Battle::Tile * next)
{
  if (next->IsEdgeTile()) return false;

  bool valid = next ? (next->IsWalkable() && (next->GetTeam() == tileStartTeam)) : false;
  
  if (next == tile) return true;

  if (valid) {
    if (next->ContainsEntityType<Gear>()) {
      if (GetMoveDirection() == Direction::left) {
        SetMoveDirection(Direction::right);
      }
      else if(GetMoveDirection() == Direction::right) {
        SetMoveDirection(Direction::left);
      }
      return false;
    }

    return true;
  }

  if (GetMoveDirection() == Direction::left) {
    SetMoveDirection(Direction::right);
  }
  else if (GetMoveDirection() == Direction::right) {
    SetMoveDirection(Direction::left);
  }

  return false;
}

void Gear::OnUpdate(double _elapsed) {
  if (tileStartTeam == Team::unknown && tile) {
    tileStartTeam = tile->GetTeam();
  }

  if (stopMoving) return;

  // May have just finished moving
  tile->AffectEntities(this);

  // Keep moving
  if (!IsSliding()) {
    if (!CanMoveTo(GetTile() + GetMoveDirection())) {
      if (CanMoveTo(GetTile() + Direction::right)) {
        SetMoveDirection(Direction::right);
      }
      else if (CanMoveTo(GetTile() + Direction::left)) {
        SetMoveDirection(Direction::left);
      }
    }

    // Now try to move
    Slide(GetTile() + GetMoveDirection(), frames(120), frames(0));
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
  Character::OnBattleStop();

    Logger::Log("Gear::OnBattleStop()");
    stopMoving = true;
}
