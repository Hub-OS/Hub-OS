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
  if (!stateStr.empty()) {
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
  }
  
  if (state != MovementState::idle) {
    auto& [_, new_pos] = CanMoveTo(GetHeading(), state, elapsed);

    // We don't care about success or not, update the best position
    setPosition(new_pos);
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

void Overworld::Actor::CollideWithQuadTree(QuadTree& sector)
{
  this->quadTree = &sector;
}

void Overworld::Actor::SetCollisionRadius(double radius)
{
  this->collisionRadius = radius;
}

void Overworld::Actor::SetInteractCallback(const std::function<void(Actor&)>& func)
{
  this->onInteractFunc = func;
}

void Overworld::Actor::Interact(Actor& with)
{
  if (this->onInteractFunc) {
    this->onInteractFunc(with);
  }
}

const std::pair<bool, sf::Vector2f> Overworld::Actor::CollidesWith(const Actor& actor, const sf::Vector2f& offset)
{
  auto delta = (getPosition() + offset) - actor.getPosition();
  float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
  float sumOfRadii = actor.collisionRadius + collisionRadius;
  auto delta_unit = sf::Vector2f(delta.x / distance, delta.y / distance);

  // suggested point of collision is the edge of the circle
  auto vec = actor.getPosition() + (delta_unit * static_cast<float>(sumOfRadii));

  // return collision status and point of potential intersection
  return { distance < sumOfRadii, vec };
}

const std::pair<bool, sf::Vector2f> Overworld::Actor::CanMoveTo(Direction dir, MovementState state, double elapsed)
{
  double px_per_s = 0;

  if (state == MovementState::running) {
    px_per_s = GetRunSpeed() * elapsed;
  }
  else if (state == MovementState::walking) {
    px_per_s = GetWalkSpeed() * elapsed;
  }

  auto currPos = getPosition();
  auto offset = MakeVectorFromDirection(dir, static_cast<float>(px_per_s));
  auto newPos = currPos + offset;

  auto& [first, second] = Split(dir);

  /**
  * Check if colliding with the map
  * This should be a cheaper check to do first 
  * before checking neighboring actors
  */
  if (map && map->GetTileAt(newPos).solid) {
    if (second != Direction::none) {
      // split vector into parts and try each individual segment
      auto diffx = offset;
      diffx.y = 0;

      auto diffy = offset;
      diffy.x = 0;

      newPos = currPos;

      if (map->GetTileAt(currPos + diffx).solid == false) {
        newPos = currPos + diffx;
      }

      if (map->GetTileAt(currPos + diffy).solid == false) {
        newPos = currPos + diffy;
      }

      return { false, newPos };
    }

    return { false, currPos }; // Otherwise, we cannot move
  }

  /**
  * If we can move forward, check neighboring actors
  */
  if (quadTree) {
    for (auto actor : quadTree->actors) {
      if (actor == this) continue;

      auto& [hit, circle_edge] = CollidesWith(*actor, offset);

      if (hit) {
        return { false, circle_edge };
      }
    }
  }

  // The referenced games did not use isometric coordinates
  // and moved uniformly in screen-space.
  // These 2 cases are quicker in isometric space because it is extremely 
  // negative or positive for both x and y.
  bool normalize = dir == Direction::up_right || dir == Direction::down_left;

  if (normalize) {
    // If we are clearly moving in these extreme directions, nerf the result
    newPos = currPos + (offset * 0.5f);
  }

  // There were no obstructions
  return { true, newPos };
}

const Overworld::QuadTree* Overworld::Actor::GetQuadTree()
{
  return quadTree;
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
  if (anims.empty()) return "";

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

// class QuadTree

std::vector<Overworld::Actor*> Overworld::QuadTree::GetActors() const
{
  return this->actors;
}
