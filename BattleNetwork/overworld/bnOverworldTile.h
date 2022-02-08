#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "bnOverworldTileType.h"
#include "bnOverworldTileShadow.h"
#include "bnOverworldCustomProperties.h"
#include "bnOverworldShapes.h"
#include "../bnDirection.h"
#include "../bnAnimation.h"

namespace Overworld
{
  class Map;

  enum class Projection { Isometric, Orthographic };

  struct Tileset {
    const std::string name;
    const unsigned int firstGid;
    const unsigned int tileCount;
    const sf::Vector2f drawingOffset;
    const sf::Vector2f alignmentOffset;
    const Projection orientation; // used for collisions
    const CustomProperties customProperties;
    std::shared_ptr<sf::Texture> texture;
    Animation animation;
  };

  struct TileMeta {
    const unsigned int id;
    const unsigned int gid;
    const sf::Vector2f drawingOffset;
    const sf::Vector2f alignmentOffset;
    const TileType::TileType type;
    const TileShadow::TileShadow shadow;
    const Direction direction;
    const CustomProperties customProperties;
    const std::vector<std::unique_ptr<Shape>> collisionShapes;
    Animation animation;
    sf::Sprite sprite;


    TileMeta(
      const Tileset& tileset,
      unsigned int id,
      unsigned int gid,
      sf::Vector2f drawingOffset,
      sf::Vector2f alignmentOffset,
      TileType::TileType type,
      TileShadow::TileShadow shadow,
      const CustomProperties& customProperties,
      std::vector<std::unique_ptr<Shape>> collisionShapes
    );
  };

  struct Tile {
    unsigned int gid;
    bool flippedHorizontal;
    bool flippedVertical;
    bool rotated;

    Tile(unsigned int gid) {
      // https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
      bool flippedHorizontal = (gid >> 31 & 1) == 1;
      bool flippedVertical = (gid >> 30 & 1) == 1;
      bool flippedAntiDiagonal = (gid >> 29 & 1) == 1;

      this->gid = gid << 3 >> 3;
      this->rotated = flippedAntiDiagonal;

      if (rotated) {
        this->flippedHorizontal = flippedVertical;
        this->flippedVertical = !flippedHorizontal;
      }
      else {
        this->flippedHorizontal = flippedHorizontal;
        this->flippedVertical = flippedVertical;
      }
    }

    bool Intersects(Map& map, float x, float y) const;
    Direction GetDirection(TileMeta& meta);
  };
}
