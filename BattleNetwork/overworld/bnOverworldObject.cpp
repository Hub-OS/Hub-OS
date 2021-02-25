#include "bnOverworldObject.h"
#include <cmath>

namespace Overworld {
  TileObject::TileObject(unsigned int id, Tile tile) : tile(tile) {
    this->id = id;
    visible = true;
    worldSprite = std::make_shared<WorldSprite>();
  }

  TileObject::TileObject(unsigned int id, unsigned int gid) : TileObject(id, Tile(gid)) {}

  bool TileObject::Intersects(Map& map, float x, float y) const {
    auto tileset = map.GetTileset(tile.gid);

    if (tileset == nullptr) {
      return false;
    }

    auto& tileMeta = *map.GetTileMeta(tile.gid);
    auto tileSize = map.GetTileSize();
    auto spriteOrigin = tileMeta.sprite.getOrigin();
    auto spriteBounds = tileMeta.sprite.getLocalBounds();

    auto iso = sf::Vector2f(x, y);
    iso.x -= this->position.x;
    iso.y -= this->position.y;
    auto ortho = map.WorldToScreen(iso);

    // rotation
    if (rotation != 0.0) {
      auto magnitude = std::hypotf(ortho.x, ortho.y);
      auto orthoRadians = std::atan2(ortho.y, ortho.x);
      auto rotationRadians = -rotation / 180.0f * M_PI;
      ortho.x = std::cos(orthoRadians + rotationRadians) * magnitude;
      ortho.y = std::sin(orthoRadians + rotationRadians) * magnitude;
    }

    // calculate offset separately as we need to transform it separately
    auto orthoOffset = -tileset->alignmentOffset;

    // not sure why this is needed
    orthoOffset.x -= tileSize.x / 2;

    // not sure where this offset comes from
    orthoOffset.x += 1;
    orthoOffset.y -= 1;

    // flip
    if (tile.flippedHorizontal) {
      orthoOffset.x *= -1;

      // starting from the right side of the image during a horizontal flip
      orthoOffset.x -= spriteBounds.width;
    }

    if (tile.flippedVertical) {
      orthoOffset.y *= -1;

      // might be similar to the flip above, but not sure
      orthoOffset.y -= tileSize.y * 2 - (spriteBounds.height + tileset->drawingOffset.y);
    }

    // skipping tile rotation, editor doesn't allow this

    // adjust for the sprite being aligned at the bottom of the tile
    orthoOffset.y -= spriteBounds.height - tileSize.y;

    ortho += orthoOffset;

    // scale
    auto scale = sf::Vector2f(size.x / spriteBounds.width, size.y / spriteBounds.height);
    ortho.x /= scale.x;
    ortho.y /= scale.y;

    iso = map.OrthoToIsometric(ortho);

    return tile.Intersects(map, iso.x, iso.y);
  }

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

    auto ortho = map.WorldToScreen(position);

    sf::Transform preTransform;
    preTransform.translate(ortho);
    preTransform.rotate(rotation);
    preTransform.translate(tileMeta->alignmentOffset + tileMeta->drawingOffset + origin);
    preTransform.translate(-ortho);
    worldSprite->SetPreTransform(preTransform);
  }
}
