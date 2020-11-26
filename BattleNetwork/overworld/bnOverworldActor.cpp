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
  validStates.clear();
  anims.clear();

  auto dir_array = {
    Direction::down, Direction::down_left, Direction::down_right,
    Direction::left, Direction::right, Direction::up,
    Direction::up_left, Direction::up_right
  };

  auto state_array = {
    MovementState::idle, MovementState::running, MovementState::walking
  };

  auto loadDirectionalAnimationThunk = [=](Direction dir, MovementState state) {
    std::string str = MovementAnimStrPrefix(state) + "_" + DirectionAnimStrSuffix(dir);
    Animation anim = Animation(path);

    if (anim.HasAnimation(str)) {
      auto& [iter, success] = anims.insert(std::make_pair(str, anim));

      if (success) {
        iter->second.SetAnimation(str);
        iter->second << Animator::Mode::Loop;
        validStates.push_back({ state, dir });
      }

      return success;
    }

    return false;
  };

  for (auto dir : dir_array) {
    for (auto state : state_array) {
      loadDirectionalAnimationThunk(dir, state);
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
  std::string stateStr = FindValidAnimState(this->heading, this->state);

  // If there is no supplied animation for the next state, ignore it
  if (!anims.begin()->second.HasAnimation(stateStr)) {
    stateStr = lastStateStr; 
  }

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

  if (currSector) {
    for (auto actor : currSector->actors) {
      if (actor == this) continue;

      if (CollidesWith(*actor)) {
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


          if (CollidesWith(*actor, diffx)) {
            setPosition(lastPos + diffx);
          }

          if (CollidesWith(*actor, diffy)) {
            setPosition(lastPos + diffy);
          }
        }
      }
    }
  }

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

  // The referenced games did not use isometric coordinates
  // and moved uniformly in screen-space
  // These 2 cases are quicker in isometric space because it is extremely 
  // negative or positive for both x and y.
  bool normalize = dir == Direction::up_right || dir == Direction::down_left;

  updateOffsetThunk(a, &offset, length);
  updateOffsetThunk(b, &offset, length);

  if (normalize) {
    offset *= 0.5f;
  }

  return offset;
}

Direction Overworld::Actor::MakeDirectionFromVector(const sf::Vector2f& vec, float threshold)
{
  Direction first = Direction::none;
  Direction second = Direction::none;

  if (vec.x < -std::fabs(threshold)) {
    first = Direction::left;
  }
  else if (vec.x > std::fabs(threshold)) {
    first = Direction::right;
  }

  if (vec.y < -std::fabs(threshold)) {
    second = Direction::up;
  }
  else if (vec.y > std::fabs(threshold)) {
    second = Direction::down;
  }

  return Join(first, second);
}

void Overworld::Actor::WatchForCollisions(QuadTree& sector)
{
  this->currSector = &sector;
}

void Overworld::Actor::SetCollisionRadius(double radius)
{
  this->collisionRadius = radius;
}

const bool Overworld::Actor::CollidesWith(const Actor& actor, const sf::Vector2f& offset)
{
  auto delta = actor.getPosition() - (getPosition() + offset);
  double distance = std::pow(delta.x, 2.0) + std::pow(delta.y, 2.0);

  return distance < std::pow(actor.collisionRadius + collisionRadius, 2.0);
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

std::string Overworld::Actor::FindValidAnimState(const Direction& dir, const MovementState& state)
{
  // Some animations do not need to be provided (like cardinal-facing left or right)
  // when others can be substituted
  //
  // If we do not have a matching state animation, find the next best one:
  // 1. Try using another vertical direction
  // 2. Try using another horizontal direction
  // Else bail

  auto hasAnimStateThunk = [this](const Direction& dir, const MovementState& state) {
    auto iter = std::find_if(validStates.begin(), validStates.end(),
      [dir, state](const AnimStatePair& pair) {
        return std::tie(dir, state) == std::tie(pair.dir, pair.movement);
      });

    return std::tuple{ iter != validStates.end(), dir, state };
  };

  auto attemptThunk = [](const std::initializer_list<std::tuple<bool, Direction, MovementState>>& ts) {
    for(auto& item : ts) {
      if (std::get<0>(item)) {
        return AnimStatePair{ std::get<2>(item), std::get<1>(item) };
      }
    }

    // bad state
    return AnimStatePair{ MovementState::size, Direction::none };
  };

  auto str = MovementAnimStrPrefix(state) + "_" + DirectionAnimStrSuffix(Isometric(dir));

  if (anims.begin()->second.HasAnimation(str) == false) {
    auto& [ud, lr] = Split(Isometric(dir));
    
    std::map<Direction, std::initializer_list<std::tuple<bool, Direction, MovementState>>> attempts = {
      {
        Direction::left,
        {
          hasAnimStateThunk(Direction::up_left, state),
          hasAnimStateThunk(Direction::down_left, state)
        }
      },

      {
        Direction::up_left,
        {
          hasAnimStateThunk(Direction::left, state),
          hasAnimStateThunk(Direction::up, state)
        }
      },

      {
        Direction::up,
        {
          hasAnimStateThunk(Direction::up_left, state),
          hasAnimStateThunk(Direction::up_right, state)
        }
      },

      {
        Direction::up_right,
        {
          hasAnimStateThunk(Direction::right, state),
          hasAnimStateThunk(Direction::up, state)
        }
      },

      {
        Direction::right,
        {
          hasAnimStateThunk(Direction::up_right, state),
          hasAnimStateThunk(Direction::down_right, state)
        }
      },

      {
        Direction::down_right,
        {
          hasAnimStateThunk(Direction::right, state),
          hasAnimStateThunk(Direction::down, state)
        }
      },

      {
        Direction::down,
        {
          hasAnimStateThunk(Direction::down_left, state),
          hasAnimStateThunk(Direction::down_right, state)
        }
      },

      {
        Direction::down_left,
        {
          hasAnimStateThunk(Direction::left, state),
          hasAnimStateThunk(Direction::down, state)
        }
      }
    };

    AnimStatePair pair = attemptThunk(attempts[Isometric(dir)]);

    // valid pair was found
    if (pair.dir != Direction::none) {
      return MovementAnimStrPrefix(pair.movement) + "_" + DirectionAnimStrSuffix(pair.dir);
    }
  }

  return str;
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
