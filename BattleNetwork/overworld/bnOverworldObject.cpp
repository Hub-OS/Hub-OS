#include "bnOverworldObject.h"
#include <math.h>

namespace Overworld {
  TileObject::TileObject(unsigned int id, Tile tile) : tile(tile) {
    this->id = id;
    visible = true;
    worldSprite = std::make_shared<WorldSprite>();
  }

  TileObject::TileObject(unsigned int id, unsigned int gid) : TileObject(id, Tile(gid)) {}

  void TileObject::Update(Map& map) {
    auto& tileMeta = map.GetTileMeta(tile.gid);
    auto tileset = map.GetTileset(tile.gid);

    if (tileMeta == nullptr || tileset == nullptr) {
      return;
    }

    if (!visible) {
      worldSprite->setTextureRect(sf::IntRect(0, 0, 0, 0));
      return;
    }

    auto origin = tileMeta->sprite.getOrigin();
    worldSprite->setTexture(tileset->texture);
    worldSprite->setTextureRect(tileMeta->sprite.getTextureRect());
    worldSprite->setOrigin(origin);

    auto horizontalMultiplier = tile.flippedHorizontal ? -1.0f : 1.0f;
    auto verticalMultiplier = tile.flippedVertical ? -1.0f : 1.0f;
    auto localBounds = worldSprite->getLocalBounds();

    worldSprite->setPosition(position);
    worldSprite->setScale(horizontalMultiplier * size.x / localBounds.width, verticalMultiplier * size.y / localBounds.height);
    worldSprite->setRotation(rotation);

    auto ortho = map.WorldToScreen(position);

    sf::Transform preTransform;
    preTransform.translate(ortho);
    preTransform.rotate(rotation);
    preTransform.translate(tileMeta->alignmentOffset + tileMeta->drawingOffset + origin);
    preTransform.translate(-ortho);
    worldSprite->SetPreTransform(preTransform);
  }
}
