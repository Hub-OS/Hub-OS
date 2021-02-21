#include "bnOverworldObject.h"
#include <math.h>

namespace Overworld {
  TileObject::TileObject(unsigned int id, Tile tile) : tile(tile) {
    this->id = id;
    visible = true;
    spriteProxy = std::make_shared<SpriteProxyNode>();
  }

  TileObject::TileObject(unsigned int id, unsigned int gid) : TileObject(id, Tile(gid)) {}

  void TileObject::Update(Map& map) {
    auto& tileMeta = map.GetTileMeta(tile.gid);

    if (tileMeta == nullptr) {
      return;
    }

    auto& sprite = spriteProxy->getSprite();

    if (!visible) {
      sprite.setTextureRect(sf::IntRect(0, 0, 0, 0));
      return;
    }

    sprite.setTexture(*tileMeta->sprite.getTexture());
    sprite.setTextureRect(tileMeta->sprite.getTextureRect());
    sprite.setOrigin(tileMeta->sprite.getOrigin());

    auto horizontalMultiplier = tile.flippedHorizontal ? -1.0f : 1.0f;
    auto verticalMultiplier = tile.flippedVertical ? -1.0f : 1.0f;
    auto localBounds = sprite.getLocalBounds();

    spriteProxy->setPosition(position);
    spriteProxy->setScale(horizontalMultiplier * size.x / localBounds.width, verticalMultiplier * size.y / localBounds.height);
    spriteProxy->setRotation(rotation);

    // offset could be subtracted directly from the origin, if it didn't mess up sprite flipping
    auto radians = rotation / 180.f * M_PI;
    auto offsetRadians = std::atan2(tileMeta->offset.y, tileMeta->offset.x);
    auto offsetLength = std::hypotf(tileMeta->offset.x, tileMeta->offset.y);

    auto offset = sf::Vector2f(
      std::cos(offsetRadians + radians) * offsetLength,
      std::sin(offsetRadians + radians) * offsetLength
    );

    spriteProxy->move(map.OrthoToIsometric(offset));
  }
}