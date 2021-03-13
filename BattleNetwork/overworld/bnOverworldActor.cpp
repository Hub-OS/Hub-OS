#include "bnOverworldActor.h"
#include "bnOverworldMap.h"
#include <cmath>

Overworld::Actor::Actor(const std::string& name) :
  name(name)
{

}

Overworld::Actor::Actor(Actor&& other) noexcept
{
  std::swap(animProgress, other.animProgress);
  std::swap(walkSpeed, other.walkSpeed);
  std::swap(runSpeed, other.runSpeed);
  std::swap(heading, other.heading);
  std::swap(anims, other.anims);
  std::swap(validStates, other.validStates);
  std::swap(state, other.state);
  std::swap(pos, other.pos);
  std::swap(name, other.name);
  std::swap(lastStateStr, other.lastStateStr);
  std::swap(onInteractFunc, other.onInteractFunc);
  std::swap(collisionRadius, other.collisionRadius);
}

Overworld::Actor::~Actor()
{
}

void Overworld::Actor::Walk(const Direction& dir, bool move)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::walking;
  this->moveThisFrame = move;
}

void Overworld::Actor::Run(const Direction& dir, bool move)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::running;
  this->moveThisFrame = move;
}

void Overworld::Actor::Face(const Direction& dir)
{
  if (dir == Direction::none) return;

  this->heading = dir;
  this->state = MovementState::idle;
}

void Overworld::Actor::Face(const Actor& actor)
{
  auto direction = MakeDirectionFromVector(actor.getPosition() - getPosition());

  Face(direction);
}

void Overworld::Actor::LoadAnimations(const Animation& animation)
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

    if (animation.HasAnimation(str)) {
      const auto& [iter, success] = anims.insert(std::make_pair(str, animation));

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

  UpdateAnimationState(0); // refresh
}

void Overworld::Actor::SetWalkSpeed(float speed)
{
  this->walkSpeed = speed;
}

void Overworld::Actor::SetRunSpeed(float speed)
{
  this->runSpeed = speed;
}

void Overworld::Actor::Rename(const std::string& newName)
{
  this->name = newName;
}

const std::string Overworld::Actor::GetName() const
{
  return this->name;
}

float Overworld::Actor::GetWalkSpeed() const
{
  return walkSpeed;
}

float Overworld::Actor::GetRunSpeed() const
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
  return getPosition() + UnitVector(GetHeading()) * collisionRadius;
}

void Overworld::Actor::Update(float elapsed, Map& map, SpatialMap& spatialMap)
{
  UpdateAnimationState(elapsed);

  if (state != MovementState::idle && moveThisFrame) {
    auto& [_, new_pos] = CanMoveTo(GetHeading(), state, elapsed, map, spatialMap);

    // We don't care about success or not, update the best position
    setPosition(new_pos);

    moveThisFrame = false;
  }
}

void Overworld::Actor::UpdateAnimationState(float elapsed) {
  std::string stateStr = FindValidAnimState(this->heading, this->state);

  if (!stateStr.empty()) {
    // If there is no supplied animation for the next state, ignore it
    if (!anims.begin()->second.HasAnimation(stateStr)) {
      stateStr = lastStateStr;
    }

    animProgress += elapsed;

    if (lastStateStr.empty() == false) {
      anims[lastStateStr].SyncTime(animProgress);
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
}

void Overworld::Actor::CollideWithMap(bool collide)
{
  collidesWithMap = collide;
}

sf::Vector2f Overworld::Actor::MakeVectorFromDirection(Direction dir, float length)
{
  sf::Vector2f offset{};

  const auto& [a, b] = Split(dir);

  auto updateOffsetThunk = [&offset, length](const Direction& dir) mutable {
    if (dir == Direction::left) {
      offset.x -= length;
    }
    else if (dir == Direction::right) {
      offset.x += length;
    }
    else if (dir == Direction::up) {
      offset.y -= length;
    }
    else if (dir == Direction::down) {
      offset.y += length;
    }
  };

  updateOffsetThunk(a);
  updateOffsetThunk(b);

  return offset;
}

Direction Overworld::Actor::MakeDirectionFromVector(const sf::Vector2f& vec)
{
  if (vec.x == 0 && vec.y == 0) {
    return Direction::none;
  }

  Direction first = vec.x < 0 ? Direction::left : Direction::right;
  Direction second = vec.y < 0 ? Direction::up : Direction::down;

  // using slope to calculate direction, graph if you want to take a look
  auto ratio = std::fabs(vec.y) / std::fabs(vec.x);

  if (ratio < 1.f / 2.f) {
    return first;
  }
  else if (ratio > 2.f) {
    return second;
  }

  return Join(first, second);
}

void Overworld::Actor::SetSolid(bool solid)
{
  this->solid = solid;
}

float Overworld::Actor::GetCollisionRadius()
{
  return collisionRadius;
}

void Overworld::Actor::SetCollisionRadius(float radius)
{
  this->collisionRadius = radius;
}

void Overworld::Actor::SetInteractCallback(const std::function<void(std::shared_ptr<Actor>)>& func)
{
  this->onInteractFunc = func;
}

void Overworld::Actor::Interact(const std::shared_ptr<Actor>& with)
{
  if (this->onInteractFunc) {
    this->onInteractFunc(with);
  }
}

const std::optional<sf::Vector2f> Overworld::Actor::CollidesWith(const Actor& actor, const sf::Vector2f& offset)
{
  auto delta = (getPosition() + offset) - actor.getPosition();
  float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
  float sumOfRadii = actor.collisionRadius + collisionRadius;

  if (distance > sumOfRadii) {
    return {};
  }

  auto delta_unit = sf::Vector2f(delta.x / distance, delta.y / distance);

  // suggested point of collision is the edge of the circle
  auto vec = actor.getPosition() + (delta_unit * sumOfRadii);

  // return collision status and point of potential intersection
  return vec;
}

const std::pair<bool, sf::Vector2f> Overworld::Actor::CanMoveTo(Direction dir, MovementState state, float elapsed, Map& map, SpatialMap& spatialMap)
{
  float px_per_s = 0;

  if (state == MovementState::running) {
    px_per_s = GetRunSpeed() * elapsed;
  }
  else if (state == MovementState::walking) {
    px_per_s = GetWalkSpeed() * elapsed;
  }

  auto currPos = getPosition();
  auto offset = MakeVectorFromDirection(dir, px_per_s);

  const auto& [first, second] = Split(dir);

  // The referenced games did not use isometric coordinates
  // and moved uniformly in screen-space.
  // These 2 cases are quicker in isometric space because it is extremely 
  // negative or positive for both x and y.
  bool shouldNormalize = dir == Direction::up_right || dir == Direction::down_left;
  auto normalizedOffset = offset;

  if (shouldNormalize) {
    // If we are clearly moving in these extreme directions, nerf the result
    normalizedOffset *= 0.5f;
  }

  auto newPos = currPos + normalizedOffset;

  auto [canMove, firstPositionResult] = CanMoveTo(newPos, map, spatialMap);

  if (!canMove && second != Direction::none) {
    // test alternate directions, this provides a sliding effect
    auto [canMoveOffsetX, offsetXResult] = CanMoveTo(sf::Vector2f(currPos.x + offset.x, currPos.y), map, spatialMap);

    if (canMoveOffsetX) {
      return { canMoveOffsetX, offsetXResult };
    }

    auto [canMoveOffsetY, offsetYResult] = CanMoveTo(sf::Vector2f(currPos.x, currPos.y + offset.y), map, spatialMap);

    if (canMoveOffsetY) {
      return { canMoveOffsetY, offsetYResult };
    }
  }

  return { canMove, firstPositionResult };
}

const std::pair<bool, sf::Vector2f> Overworld::Actor::CanMoveTo(sf::Vector2f newPos, Map& map, SpatialMap& spatialMap) {
  auto currPos = getPosition();
  auto offset = newPos - currPos;
  auto dir = MakeDirectionFromVector(offset);

  /**
  * Check if colliding with the map
  * This should be a cheaper check to do first
  * before checking neighboring actors
  */

  auto layer = GetLayer();

  if (collidesWithMap && layer >= 0 && map.GetLayerCount() > layer) {
    auto tileSize = sf::Vector2f(map.GetTileSize());
    tileSize.x *= .5f;

    if (!map.CanMoveTo(newPos.x / tileSize.x, newPos.y / tileSize.y, layer)) {
      return { false, currPos }; // Otherwise, we cannot move
    }
  }

  /**
  * If we can move forward, check neighboring actors
  */
  if (solid) {
    for (auto actor : spatialMap.GetNeighbors(*this)) {
      if (actor.get() == this || !actor->solid) continue;

      auto collision = CollidesWith(*actor, offset);

      if (collision) {
        return { false, collision.value() };
      }
    }
  }

  // There were no obstructions
  return { true, newPos };
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
    for (auto& item : ts) {
      if (std::get<0>(item)) {
        return AnimStatePair{ std::get<2>(item), std::get<1>(item) };
      }
    }

    // bad state
    return AnimStatePair{ MovementState::size, Direction::none };
  };

  auto str = MovementAnimStrPrefix(state) + "_" + DirectionAnimStrSuffix(Isometric(dir));

  if (anims.begin()->second.HasAnimation(str) == false) {
    const auto& [ud, lr] = Split(Isometric(dir));

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
