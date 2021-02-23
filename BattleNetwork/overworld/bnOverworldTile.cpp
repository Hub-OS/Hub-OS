#include "bnOverworldTile.h"

#include "bnOverworldMap.h"
#include <SFML/Graphics.hpp>

namespace Overworld {
  bool Tile::Intersects(Map& map, float x, float y) const {
    auto tileset = map.GetTileset(gid);

    if (!tileset) {
      return true;
    }

    sf::Vector2f testPosition = sf::Vector2f(x, y);

    // convert to orthogonal to simplify transformations
    testPosition = map.WorldToScreen(testPosition);
    auto tileSize = map.GetTileSize();

    if (rotated) {
      auto tileCenter = sf::Vector2f(0, tileSize.y / 2);

      testPosition -= tileCenter;
      // rotate position counter clockwise
      testPosition = sf::Vector2(testPosition.y, -testPosition.x);
      testPosition += tileCenter;
    }

    if (flippedHorizontal) {
      testPosition.x *= -1;
    }

    if (flippedVertical) {
      testPosition.y = tileSize.y - testPosition.y;
    }

    auto& tileMeta = *map.GetTileMeta(gid);

    testPosition -= tileMeta.drawingOffset;

    if (tileset->orientation == Projection::Orthographic) {
      // tiled uses position on sprite with orthographic projection
      auto spriteBounds = tileMeta.sprite.getLocalBounds();

      testPosition.x += tileSize.x / 2;
      testPosition.y += spriteBounds.height - tileSize.y;
    }
    else {
      // isometric orientation
      testPosition = map.OrthoToIsometric(testPosition);
    }

    for (auto& shape : tileMeta.collisionShapes) {
      if (shape->Intersects(testPosition.x, testPosition.y)) {
        return true;
      }
    }

    return false;
  }
}