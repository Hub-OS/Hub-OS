#include "bnOverworldActor.h"
#include "bnOverworldMap.h"

Overworld::Actor::Actor(const std::string& name) :
  name(name)
{

}

Overworld::Actor::~Actor()
{
}

void Overworld::Actor::Walk(const Direction& dir)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::walking;
}

void Overworld::Actor::Run(const Direction& dir)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::running;
}

void Overworld::Actor::Face(const Direction& dir)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::idle;
}

void Overworld::Actor::LoadAnimations(const std::string& path)
{
  auto dir_array = {
    Direction::down, Direction::down_left, Direction::down_right,
    Direction::left, Direction::right, Direction::up,
    Direction::up_left, Direction::up_right
  };

  auto state_array = {
    MovementState::idle, MovementState::running, MovementState::walking
  };

  for (auto dir : dir_array) {
    for (auto state : state_array) {
      std::string str = MovementAnimStrPrefix(state) + "_" + DirectionAnimStrSuffix(dir);
      auto& [iter, success] = anims.insert(std::make_pair(str, Animation(path)));

      if (success) {
        iter->second.SetAnimation(str);
        iter->second << Animator::Mode::Loop;
      }
    }
  }
}

void Overworld::Actor::SetWalkSpeed(const double speed)
{
  this->walkSpeed = speed;
}

void Overworld::Actor::SetRunSpeed(const double speed)
{
  this->runSpeed = speed;
}

const std::string Overworld::Actor::GetName() const
{
  return this->name;
}

const double Overworld::Actor::GetWalkSpeed() const
{
  return walkSpeed;
}

const double Overworld::Actor::GetRunSpeed() const
{
  return runSpeed;
}

std::string Overworld::Actor::CurrentAnimStr()
{
  return MovementAnimStrPrefix(this->state) + "_" + DirectionAnimStrSuffix(Isometric(this->heading));
}

const Direction Overworld::Actor::GetHeading() const
{
  return this->heading;
}

sf::Vector2f Overworld::Actor::PositionInFrontOf() const
{
    return getPosition() + MakeVectorFromDirection(GetHeading(), 2.0f);
}

void Overworld::Actor::Update(double elapsed)
{
  std::string stateStr = CurrentAnimStr();
  animProgress += elapsed;

  if (lastStateStr.empty() == false) {
    anims[lastStateStr].SyncTime(static_cast<float>(animProgress));
    anims[lastStateStr].Refresh(getSprite());
  }

  if (lastStateStr != stateStr) {
    animProgress = 0;
    anims[stateStr].SyncTime(0);

    // we have changed states
    anims[stateStr].Refresh(getSprite());
    lastStateStr = stateStr;
  }

  double px_per_s = 0;

  if (state == MovementState::running) {
    px_per_s = GetRunSpeed()*elapsed;
  }
  else if (state == MovementState::walking) {
    px_per_s = GetWalkSpeed()*elapsed;
  }

  auto lastPos = getPosition();

  // moves in our heading direction
  move(MakeVectorFromDirection(GetHeading(), static_cast<float>(px_per_s)));

  /**
  * After updating the actor, we are moved to a new location
  * We do a post-check to see if we collided with anything marked solid
  * If so, we adjust the displacement or halt the actor
  **/

  auto newPos = getPosition();

  if (map && map->GetTileAt(newPos).solid) {
    auto diff = newPos - lastPos;
    auto& [first, second] = Split(GetHeading());

    setPosition(lastPos);

    if (second != Direction::none) {
      setPosition(lastPos);

      // split vector into parts and try each individual segment
      auto diffx = diff;
      diffx.y = 0;

      auto diffy = diff;
      diffy.x = 0;

      if (map->GetTileAt(lastPos + diffx).solid == false) {
        setPosition(lastPos + diffx);
      }

      if (map->GetTileAt(lastPos + diffy).solid == false) {
        setPosition(lastPos + diffy);
      }
    }
  }
}

void Overworld::Actor::CollideWithMap(Map& map)
{
  this->map = &map;
}

sf::Vector2f Overworld::Actor::MakeVectorFromDirection(Direction dir, float length)
{
  sf::Vector2f offset{};

  auto& [a, b] = Split(dir);

  auto updateOffsetThunk = [](const Direction& dir, sf::Vector2f* vec, float value) {
    if (dir == Direction::left) {
      vec->x -= value;
    }
    else if (dir == Direction::right) {
      vec->x += value;
    }
    else if (dir == Direction::up) {
      vec->y -= value;
    }
    else if (dir == Direction::down) {
      vec->y += value;
    }
  };

  updateOffsetThunk(a, &offset, length);
  updateOffsetThunk(b, &offset, length);

  return offset;
}

std::string Overworld::Actor::MovementAnimStrPrefix(const MovementState& state)
{
  switch (state) {
  case MovementState::idle:
    return "IDLE";
  case MovementState::running:
    return "RUN";
  case MovementState::walking:
    return "WALK";
  }

  return "IDLE"; // default is IDLE
}

std::string Overworld::Actor::DirectionAnimStrSuffix(const Direction& dir)
{
  switch (dir) {
  case Direction::up:
    return "U";
  case Direction::left:
    return "L";
  case Direction::down:
    return "D";
  case Direction::right:
    return "R";
  case Direction::up_left:
    return "UL";
  case Direction::up_right:
    return "UR";
  case Direction::down_left:
    return "DL";
  case Direction::down_right:
    return "DR";
  default:
    return "D"; // silence warnings
  }

  return "D"; // default is down
}
