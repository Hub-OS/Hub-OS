#include "bnOverworldTile.h"

#include "bnOverworldMap.h"
#include <SFML/Graphics.hpp>

namespace Overworld {
  TileMeta::TileMeta(
    const Tileset& tileset,
    unsigned int id,
    unsigned int gid,
    sf::Vector2f drawingOffset,
    sf::Vector2f alignmentOffset,
    TileType::TileType type,
    TileShadow::TileShadow shadow,
    const CustomProperties& customProperties,
    std::vector<std::unique_ptr<Shape>> collisionShapes
  ) :
    id(id),
    gid(gid),
    drawingOffset(drawingOffset),
    alignmentOffset(alignmentOffset),
    type(type),
    shadow(shadow),
    direction(FromString(customProperties.GetProperty("direction"))),
    customProperties(customProperties),
    collisionShapes(std::move(collisionShapes))
  {
    sprite.setTexture(*tileset.texture);
    animation = tileset.animation;
    animation << to_string(id) << Animator::Mode::Loop;
    animation.Refresh(sprite);
  }

  bool Tile::Intersects(Map& map, float x, float y) const {
    auto tileset = map.GetTileset(gid);

    if (!tileset) {
      return true;
    }

    sf::Vector2f testPosition = sf::Vector2f(x, y);

    // convert to orthogonal to simplify transformations
    testPosition = map.WorldToScreen(testPosition);

    auto tileMeta = map.GetTileMeta(gid);
    auto spriteBounds = tileMeta->sprite.getLocalBounds();
    auto tileSize = map.GetTileSize();

    if (rotated) {
      auto tileCenter = sf::Vector2f(0, (float)(tileSize.y / 2));

      testPosition -= tileCenter;
      // rotate position counter clockwise
      testPosition = sf::Vector2(testPosition.y, -testPosition.x);
      testPosition += tileCenter;
    }

    if (flippedHorizontal) {
      testPosition.x *= -1;
    }

    if (flippedVertical) {
      testPosition.y = spriteBounds.height - testPosition.y;
    }

    testPosition -= tileMeta->drawingOffset;

    if (tileset->orientation == Projection::Orthographic) {
      // tiled uses position on sprite with orthographic projection

      testPosition.x += static_cast<float>(tileSize.x / 2);
      testPosition.y += spriteBounds.height - tileSize.y;
    }
    else {
      // isometric orientation
      testPosition = map.ScreenToWorld(testPosition);
    }

    for (auto& shape : tileMeta->collisionShapes) {
      if (shape->Intersects(testPosition.x, testPosition.y)) {
        return true;
      }
    }

    return false;
  }

  Direction Tile::GetDirection(TileMeta& meta) {
    auto direction = meta.direction;

    if (flippedHorizontal) {
      direction = FlipHorizontal(direction);
    }

    if (flippedVertical) {
      direction = FlipVertical(direction);
    }

    return direction;
  }
}