#include "bnOverworldSpatialMap.h"

#include <algorithm>
#include <functional> 

namespace Overworld {
  SpatialMap::SpatialMap() {
    // default based on tileWidth / 2
    // changes dynamically so shouldn't matter too much
    this->chunkLength = 32;
  }

  void SpatialMap::AddActor(std::shared_ptr<Actor> actor) {
    actors.insert(actor);
  }

  void SpatialMap::RemoveActor(std::shared_ptr<Actor> actor) {
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
    auto chunkHash = GetHash(x / chunkLength, y / chunkLength);

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
    const std::function<void(size_t)> callback
  ) {
    auto pos = actor.getPosition();
    auto radius = actor.GetCollisionRadius() / chunkLength;
    auto x = pos.x / chunkLength;
    auto y = pos.y / chunkLength;

    size_t startX = x - radius;
    size_t startY = y - radius;
    size_t endX = x + radius + 1;
    size_t endY = y + radius + 1;

    for (size_t i = startX; i < endX; i++) {
      for (size_t j = startY; j < endY; j++) {
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
