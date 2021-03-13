#pragma once

#include "bnOverworldActor.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Overworld {
  class Actor;

  class SpatialMap {
  public:
    SpatialMap();

    // automatically handled by Overworld::SceneBase AddActor/RemoveActor
    void AddActor(const std::shared_ptr<Actor>& actor);
    void RemoveActor(const std::shared_ptr<Actor>& actor);
    std::vector<std::shared_ptr<Actor>> GetChunk(float x, float y);
    std::unordered_set<std::shared_ptr<Actor>> GetNeighbors(Actor& actor);

    void Update();

  private:
    std::unordered_set<std::shared_ptr<Actor>> actors;
    float chunkLength;
    std::unordered_map<size_t, std::vector<std::shared_ptr<Actor>>> chunks;
  };
}
