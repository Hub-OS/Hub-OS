#pragma once

#include "bnOverworldSpatialMap.h"
#include "../bnDrawWindow.h"

#include <memory>
#include <functional>

namespace Overworld {
  class Actor;
  class SpatialMap;
  class Map;

  class PathController {
  public:
    void ControlActor(std::shared_ptr<Actor> actor);
    void Update(double elapsed, SpatialMap& spatialMap);
    void AddPoint(sf::Vector2f point);
    void AddWait(const frame_time_t& frames);
    void ClearPoints();
    void InterruptUntil(const std::function<bool()>& condition);

    private:
      std::shared_ptr<Actor> actor;
      std::queue<std::function<bool(float, SpatialMap&)>> commands;
      std::function<bool()> interruptCondition;
  };
}