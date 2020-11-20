#include "bnOverworldActor.h"

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

  sf::Vector2f offset{};

  auto& [a, b] = Split(GetHeading());

  auto updateOffsetThunk = [](const Direction& dir, sf::Vector2f* vec, double value) {
    if (dir == Direction::left) {
      vec->x -= static_cast<float>(value);
    }
    else if (dir == Direction::right) {
      vec->x += static_cast<float>(value);
    }
    else if (dir == Direction::up) {
      vec->y -= static_cast<float>(value);
    }
    else if (dir == Direction::down) {
      vec->y += static_cast<float>(value);
    }
  };

  updateOffsetThunk(a, &offset, px_per_s);
  updateOffsetThunk(b, &offset, px_per_s);

  move(offset);
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
