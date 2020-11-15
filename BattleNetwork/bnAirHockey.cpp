#include "bnAirHockey.h"
#include "bnMobMoveEffect.h"
#include "bnParticleImpact.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

AirHockey::AirHockey(Field* field, Team team, int damage, int moveCount) :
  Spell(field, team), damage(damage), moveCount(moveCount)
{
  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/puck.png"));
  setOrigin(getLocalBounds().width / 2.f, 16.f);
  setScale(2.f, 2.f);

  if (team == Team::blue) {
    dir = Direction::down_left;
  }
  else if (team == Team::red) {
    dir = Direction::down_right;
  }

  auto props = GetHitboxProperties();
  props.damage = damage;
  props.flags |= Hit::recoil | Hit::shake | Hit::breaking;
  SetHitboxProperties(props);
}

AirHockey::~AirHockey()
{
}

void AirHockey::OnUpdate(float _elapsed)
{
  // puck does not spawn hitbox on broken or empty tiles
  bool isOverHole = GetTile()->IsHole();

  if (!isOverHole) {
    GetTile()->AffectEntities(this);
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

      //try moving
      SlideToTile(true);
      SetSlideTime(sf::seconds(frames(4).asSeconds()));
      Move(dir);

      if (reflecting) {
        // the new dir was recalculated by 
        // CanMoveTo() and `reflecting`
        // was set to true to signal this
        SlideToTile(true);
        SetSlideTime(sf::seconds(frames(4).asSeconds()));
        Move(dir);
        reflecting = false;
      }

      FinishMove();

      if (--moveCount < 0) {
        Delete();
      }
    }
  }

  HighlightTile(Battle::Tile::Highlight::solid);
  setPosition(tileOffset + GetTile()->getPosition());
}

void AirHockey::Attack(Character* _entity)
{
  if (_entity->Hit(GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT);
  }
}

bool AirHockey::CanMoveTo(Battle::Tile* next)
{
  bool isOnOtherTeam = lastTileTeam != next->GetTeam() && Teammate(next->GetTeam());

  if (next->GetState() == TileState::hidden || isOnOtherTeam) {
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
  start.AffectEntities(this);
}

void AirHockey::OnDelete()
{
  auto* fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, *GetTile());
  fx->SetOffset(tileOffset);
  Remove();
}

void AirHockey::OnCollision()
{
  auto* particle = new ParticleImpact(ParticleImpact::Type::blue);
  particle->SetOffset(tileOffset);

  GetField()->AddEntity(*particle, *GetTile());
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
    Battle::Tile* new_tile = &(tile + dir);
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