#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>

#include "bnOverworldTile.h"
#include "bnOverworldMap.h"
#include "../bnSpriteProxyNode.h"

namespace Overworld {
  struct Tile;

  class MapObject {
  public:
    unsigned int id;
    std::string name;
    bool visible;
    sf::Vector2f position;
    sf::Vector2f size;
    float rotation;

    std::shared_ptr<WorldSprite> GetWorldSprite() { return worldSprite; }

    virtual ~MapObject() = default;

  protected:
    std::shared_ptr<WorldSprite> worldSprite;

    MapObject() = default;

    friend Map;
  };

  class TileObject : public MapObject {
  public:
    Overworld::Tile tile;

    TileObject(unsigned int id, Overworld::Tile tile);
    TileObject(unsigned int id, unsigned int gid);

    void Update(Map& map);
  };
}