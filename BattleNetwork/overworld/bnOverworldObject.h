#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>

#include "bnOverworldSprite.h"
#include "bnOverworldShapes.h"
#include "bnOverworldTile.h"
#include "bnOverworldMap.h"
#include "../bnSpriteProxyNode.h"

namespace Overworld {
  struct Tile;
  class Map;

  class MapObject {
  public:
    unsigned int id;
    std::string name;
    bool visible{ true };
    bool solid{};
    sf::Vector2f position;
    sf::Vector2f size;
    float rotation;

    virtual bool Intersects(Map& map, float x, float y) const { return false; }
    std::shared_ptr<WorldSprite> GetWorldSprite() { return worldSprite; }

    virtual ~MapObject() = default;

  protected:
    std::shared_ptr<WorldSprite> worldSprite;

    MapObject() = default;

    friend Map;
  };

  class ShapeObject : public MapObject {
  public:
    std::unique_ptr<Overworld::Shape> shape;

    ShapeObject(unsigned int id, std::unique_ptr<Overworld::Shape> shape);

    bool Intersects(Map& map, float x, float y) const override;
  };

  class TileObject : public MapObject {
  public:
    Overworld::Tile tile;

    TileObject(unsigned int id, Overworld::Tile tile);
    TileObject(unsigned int id, unsigned int gid);

    bool Intersects(Map& map, float x, float y) const override;

    void Update(Map& map);
  };
}
