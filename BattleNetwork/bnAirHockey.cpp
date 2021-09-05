#include "bnAirHockey.h"
#include "bnMobMoveEffect.h"
#include "bnParticleImpact.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

AirHockey::AirHockey(Field* field, Team team, int damage, int moveCount) :
  Spell(team), 
  damage(damage), 
  moveCount(moveCount)
{
  setTexture(Textures().LoadTextureFromFile("resources/spells/puck.png"));
  setOrigin(getLocalBounds().width / 2.f, 16.f);
  setScale(2.f, 2.f);
  SetFloatShoe(true);

  if (team == Team::blue) {
    dir = Direction::down_left;
  }
  else if (team == Team::red) {
    dir = Direction::down_right;
  }

  launchSfx = Audio().LoadFromFile("resources/sfx/pucklaunch.ogg");
  bounceSfx = Audio().LoadFromFile("resources/sfx/puckhit.ogg");

  auto props = GetHitboxProperties();
  props.damage = damage;
  props.flags |= Hit::flinch | Hit::shake | Hit::breaking;
  props.flags &= ~Hit::flash;
  SetHitboxProperties(props);
}

AirHockey::~AirHockey()
{
}

void AirHockey::OnUpdate(double _elapsed)
{
  // puck does not spawn hitbox on broken or empty tiles
  bool isOverHole = GetTile()->IsHole();

  if (!isOverHole) {
    GetTile()->AffectEntities(*this);
  }
  else {
    Delete();
  }

  if (!IsSliding()) {
    if (dir == Direction::none) {
      Delete();
    }
    else {
      lastTileTeam = GetTile()->GetTeam();

      if (CanMoveTo(*GetTile() + dir)) {
        if (reflecting) {
          reflecting = false;
          Audio().Play(bounceSfx, AudioPriority::low);
        }
        // the new dir was recalculated by 
        // CanMoveTo() and `reflecting`
        // was set to true to signal this
        auto onBegin = [this] {
          if (--this->moveCount < 0) {
            Delete();
          }
        };

        Slide(GetTile() + dir, frames(4), frames(0), ActionOrder::voluntary, onBegin);
      }
    }
  }

  HighlightTile(Battle::Tile::Highlight::solid);
}

void AirHockey::Attack(std::shared_ptr<Character> _entity)
{
  if (_entity->Hit(GetHitboxProperties())) {
    Audio().Play(AudioType::HURT);
  }
}

bool AirHockey::CanMoveTo(Battle::Tile* next)
{
  if (!next) return false;

  bool isOnOtherTeam = lastTileTeam != next->GetTeam() && Teammate(next->GetTeam());
  bool hidden = next->GetState() == TileState::hidden || next->IsEdgeTile();
  if (hidden || isOnOtherTeam) {
    if (!reflecting) {
      reflecting = true;
      dir = Bounce(dir, *next);
    }
    return false;
  }

  return true;
}

void AirHockey::OnSpawn(Battle::Tile& start)
{
  // Even on spawn, the hitbox is active
  start.AffectEntities(*this);
  Audio().Play(launchSfx, AudioPriority::low);
}

void AirHockey::OnDelete()
{
  auto fx = std::make_shared<MobMoveEffect>();
  auto result = GetField()->AddEntity(fx, *GetTile());

  if (result != Field::AddEntityStatus::deleted) {
    fx->SetOffset(tileOffset);
  }

  Remove();
}

void AirHockey::OnCollision(const std::shared_ptr<Character>)
{
  auto particle = std::make_shared<ParticleImpact>(ParticleImpact::Type::blue);
  particle->SetOffset(tileOffset);

  GetField()->AddEntity(particle, *GetTile());
}

// Bounce flips x directional value and tests, then y, then both. If all 3 fail, do not bounce
const Direction AirHockey::Bounce(const Direction& dir, Battle::Tile& next)
{
  auto& tile = *GetTile();

  auto [a, b] = Split(dir);
  Direction x{}, y{};

  if (a == Direction::up || a == Direction::down) {
    x = b;
    y = a;
  }
  else {
    x = a;
    y = b;
  }

  Direction flip_x = Reverse(x);
  Direction flip_y = Reverse(y);

  auto testMoveThunk = [this](Battle::Tile& tile, const Direction& dir) -> bool {
    Battle::Tile* new_tile = (tile + dir);
    return CanMoveTo(new_tile);
  };

  auto testAllMovesThunk = [=] (Battle::Tile& tile) -> Direction {
    Direction x_flip_y = Join(x, flip_y);
    Direction flip_x_y = Join(flip_x, y);
    Direction flip_all = Join(flip_x, flip_y);

    if (testMoveThunk(tile, x_flip_y)) {
      return x_flip_y;
    } else if (testMoveThunk(tile, flip_x_y)) {
      return flip_x_y;
    } else if (testMoveThunk(tile, flip_all)) {
      return flip_all;
    }
    
    // else
    return Direction::none;
  };

  return testAllMovesThunk(tile);
}