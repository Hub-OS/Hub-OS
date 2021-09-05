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
  SetDirection(startDir);
  HighlightTile(Battle::Tile::Highlight::solid);

  animation = CreateComponent<AnimationComponent>(weak_from_base<Character>());
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

  AddDefenseRule((indestructable = std::make_shared<DefenseIndestructable>(true)));
  AddDefenseRule((nodrag = std::make_shared<DefenseNodrag>()));

  stopMoving = true;
}

Gear::~Gear() {
  RemoveDefenseRule(indestructable);
  RemoveDefenseRule(nodrag);
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

  if (stopMoving) return;

  // May have just finished moving
  tile->AffectEntities(*this);

  // Keep moving
  if (!IsSliding()) {
    if (!CanMoveTo(GetTile() + GetDirection())) {
      if (CanMoveTo(GetTile() + Direction::right)) {
        SetDirection(Direction::right);
      }
      else if (CanMoveTo(GetTile() + Direction::left)) {
        SetDirection(Direction::left);
      }
    }

    // Now try to move
    Slide(GetTile() + GetDirection(), frames(120), frames(0));
  }
}

void Gear::OnDelete() {
    Remove();
}

void Gear::Attack(std::shared_ptr<Character> other) {
  auto isObstacle = dynamic_cast<Obstacle*>(other.get());

  if (isObstacle) {
    auto props = Hit::DefaultProperties;
    props.damage = 9999;
    isObstacle->Hit(props);
    hit = true;
    return;
  }

  auto isCharacter = dynamic_cast<Character*>(other.get());

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
