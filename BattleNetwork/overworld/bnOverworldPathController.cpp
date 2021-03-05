#include "bnOverworldPathController.h"
#include "bnOverworldActor.h"
#include <cmath>

void Overworld::PathController::ControlActor(std::shared_ptr<Actor> actor)
{
  this->actor = actor;
}

void Overworld::PathController::Update(double elapsed, Map& map, SpatialMap& spatialMap)
{
  if (interruptCondition && !interruptCondition()) return;

  // free interrupt condition
  interruptCondition = nullptr;

  if (commands.empty()) {
    return;
  }

  if(commands.front()(elapsed, map, spatialMap)) {
    commands.pop();
  }
}

void Overworld::PathController::AddPoint(sf::Vector2f dest)
{
  auto closure = [=](float elapsed, Map& map, SpatialMap& spatialMap) {
    auto vec = dest - actor->getPosition();
    float mag = std::sqrt((vec.x * vec.x) + (vec.y * vec.y));

    if (mag <= 1.f) {
      return true;
    }

    Direction dir = Actor::MakeDirectionFromVector(vec);

    auto& [success, _] = actor->CanMoveTo(dir, Actor::MovementState::walking, elapsed, map, spatialMap);

    if (success) {
      actor->Walk(dir);
    }
    else {
      actor->Face(dir);
    }

    return false;
  };

  commands.push(closure);
}

void Overworld::PathController::AddWait(const frame_time_t& frames)
{
  frame_time_t remaining_frames = frames;

  auto closure = [=](float elapsed, Map& map, SpatialMap& spatialMap) mutable {
    remaining_frames -= from_seconds(elapsed);

    if (remaining_frames <= ::frames(0)) {
      return true;
    }

    return false;
  };

  commands.push(closure);
}

void Overworld::PathController::ClearPoints() {
  commands = {};
}

void Overworld::PathController::InterruptUntil(const std::function<bool()>& condition)
{
  interruptCondition = condition;
}
