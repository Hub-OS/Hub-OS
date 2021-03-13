#include "bnOverworldSpatialMap.h"

#include <algorithm>
#include <functional> 

namespace Overworld {
  SpatialMap::SpatialMap() {
    // default based on tileWidth / 2
    // changes dynamically so shouldn't matter too much
    this->chunkLength = 32;
  }

  void SpatialMap::AddActor(const std::shared_ptr<Actor>& actor) {
    actors.insert(actor);
  }

  void SpatialMap::RemoveActor(const std::shared_ptr<Actor>& actor) {
    auto iterEnd = actors.end();
    auto iter = find(actors.begin(), iterEnd, actor);

    if (iter != iterEnd) {
      actors.erase(iter);
    }
  }

  static size_t GetHash(size_t x, size_t y) {
    return (y << (sizeof(size_t) / 2)) + x;
  }

  std::vector<std::shared_ptr<Actor>> SpatialMap::GetChunk(float x, float y) {
    auto chunkHash = GetHash((size_t)(x / chunkLength), (size_t)(y / chunkLength));

    auto it = chunks.find(chunkHash);

    if (it == chunks.end()) {
      auto& chunk = it->second;
      return {};
    }

    return it->second;

  }

  // using template for handling differing const requirements
  static void ForEachOverlappingChunk(
    float chunkLength,
    Actor& actor,
    const std::function<void(size_t)>& callback
  ) {
    auto pos = actor.getPosition();
    auto radius = actor.GetCollisionRadius() / chunkLength;
    auto x = pos.x / chunkLength;
    auto y = pos.y / chunkLength;

    int startX = static_cast<int>(x - radius);
    int startY = static_cast<int>(y - radius);
    int endX = static_cast<int>(x + radius + 1);
    int endY = static_cast<int>(y + radius + 1);

    for (int i = startX; i < endX; i++) {
      for (int j = startY; j < endY; j++) {
        callback(GetHash(i, j));
      }
    }
  };

  std::unordered_set<std::shared_ptr<Actor>> SpatialMap::GetNeighbors(Actor& actor) {
    std::unordered_set<std::shared_ptr<Actor>> neighbors;
    auto actorPtr = &actor;
    auto endIt = chunks.end();

    ForEachOverlappingChunk(chunkLength, actor, [&](size_t chunkHash) mutable {
      auto it = chunks.find(chunkHash);

      if (it == endIt) {
        return;
      }

      auto& chunk = it->second;

      for (auto& other : chunk) {
        if (actorPtr != other.get()) {
          neighbors.emplace(other);
        }
      }
    });

    return neighbors;
  }

  void SpatialMap::Update() {
    chunks.clear();

    auto oldChunkLength = chunkLength;

    for (auto& actor : actors) {
      auto collisionRadius = actor->GetCollisionRadius();

      if (collisionRadius > chunkLength) {
        chunkLength = collisionRadius;
      }

      ForEachOverlappingChunk(oldChunkLength, *actor, [&](size_t chunkHash) mutable {
        auto it = chunks.find(chunkHash);

        if (it != chunks.end()) {
          auto& chunk = it->second;
          chunk.push_back(actor);
        }
        else {
          chunks.emplace(chunkHash, std::vector<std::shared_ptr<Actor>>({ actor }));
        }
      });
    }
  }
}
