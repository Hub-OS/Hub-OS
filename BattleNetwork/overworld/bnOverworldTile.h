#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "bnOverworldShapes.h"
#include "bnOverworldObject.h"
#include "../bnAnimation.h"

namespace Overworld
{
  struct Tileset {
    const std::string name;
    const unsigned int firstGid;
    const unsigned int tileCount;
    const sf::Vector2f offset;
    std::shared_ptr<sf::Texture> texture;
    Animation animation;
  };

  struct TileMeta {
    const unsigned int id;
    const unsigned int gid;
    const sf::Vector2f offset;
    Animation animation;
    sf::Sprite sprite;
    std::vector<std::unique_ptr<Shape>> collisionShapes;


    TileMeta(unsigned int id, unsigned int gid, sf::Vector2f offset)
      : id(id), gid(gid), offset(offset) {}
  };

  struct Tile {
    unsigned int gid;
    bool flippedHorizontal;
    bool flippedVertical;
    bool rotated;

    Tile(unsigned int gid) {
      // https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
      this->gid = gid << 3 >> 3;
      flippedHorizontal = (gid >> 31 & 1) == 1;
      flippedVertical = (gid >> 30 & 1) == 1;
      rotated = (gid >> 29 & 1) == 1;
    }
  };
}
