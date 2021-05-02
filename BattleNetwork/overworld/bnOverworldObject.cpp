#include "bnOverworldObject.h"
#include <Swoosh/Ease.h>
#include <math.h>

namespace Overworld {
  ShapeObject::ShapeObject(unsigned int id, std::unique_ptr<Overworld::Shape> shape) {
    this->id = id;
    this->shape = std::move(shape);
  }

  bool ShapeObject::Intersects(Map& map, float x, float y) const {
    auto relativePosition = sf::Vector2f(x, y) - this->position;
    auto relativeOrtho = map.WorldToScreen(relativePosition);

    return shape->Intersects(relativeOrtho.x, relativeOrtho.y);
  }

  TileObject::TileObject(unsigned int id, Tile tile) : tile(tile) {
    this->id = id;
    solid = true;
    worldSprite = std::make_shared<WorldSprite>();
  }

  TileObject::TileObject(unsigned int id, unsigned int gid) : TileObject(id, Tile(gid)) {}

  bool TileObject::Intersects(Map& map, float x, float y) const {
    auto tileset = map.GetTileset(tile.gid);

    if (tileset == nullptr) {
      return false;
    }

    auto tileMeta = map.GetTileMeta(tile.gid);
    auto tileSize = map.GetTileSize();
    auto spriteOrigin = tileMeta->sprite.getOrigin();
    auto spriteBounds = tileMeta->sprite.getLocalBounds();

    auto iso = sf::Vector2f(x, y);
    iso.x -= this->position.x;
    iso.y -= this->position.y;
    auto ortho = map.WorldToScreen(iso);

    // rotation
    if (rotation != 0.0) {
      auto magnitude = std::hypotf(ortho.x, ortho.y);
      auto orthoRadians = std::atan2(ortho.y, ortho.x);
      auto rotationRadians = -rotation / 180.0f * static_cast<float>(swoosh::ease::pi);
      ortho.x = std::cos(orthoRadians + rotationRadians) * magnitude;
      ortho.y = std::sin(orthoRadians + rotationRadians) * magnitude;
    }

    // scale
    auto scale = sf::Vector2f(size.x / spriteBounds.width, size.y / spriteBounds.height);
    ortho.x /= scale.x;
    ortho.y /= scale.y;

    // calculate offset separately as we need to transform it separately
    auto orthoOffset = -tileset->alignmentOffset;

    // not sure why this is needed
    // something to do with being centered at tileSize.x / 2
    orthoOffset.x -= tileSize.x / 2;

    // not sure where this offset comes from
    orthoOffset.x += 1;
    orthoOffset.y -= 1;

    // flip
    if (tile.flippedHorizontal) {
      orthoOffset.x *= -1;

      // starting from the right side of the image during a horizontal flip
      orthoOffset.x -= spriteBounds.width + tileset->alignmentOffset.x + tileset->drawingOffset.x;

      // account for being incorrect?
      orthoOffset.x -= (tileSize.x / 2 - (spriteBounds.width + tileset->drawingOffset.x)) / 2;

      // what??
      orthoOffset.x -= tileSize.x / 2 * (tileset->alignmentOffset.x / spriteBounds.width);
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

    iso = map.OrthoToIsometric(ortho);

    return tile.Intersects(map, iso.x, iso.y);
  }

  void TileObject::Update(Map& map) {
    auto tileMeta = map.GetTileMeta(tile.gid);
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
    auto scale = sf::Vector2f(size.x / localBounds.width, size.y / localBounds.height);

    worldSprite->setPosition(position);
    worldSprite->setScale(horizontalMultiplier * scale.x, verticalMultiplier * scale.y);

    auto ortho = map.WorldToScreen(position);

    sf::Transform preTransform;
    preTransform.translate(ortho);
    preTransform.rotate(rotation);
    preTransform.scale(scale.x, scale.y);
    preTransform.translate(tileMeta->alignmentOffset + tileMeta->drawingOffset + origin);
    preTransform.scale(1.0f / scale.x, 1.0f / scale.y);
    preTransform.translate(-ortho);
    worldSprite->SetPreTransform(preTransform);
  }


  static void AdoptAttributes(MapObject& object, const XMLElement& element) {
    object.name = element.GetAttribute("name");
    object.type = element.GetAttribute("type");
    object.position = sf::Vector2f(
      element.GetAttributeFloat("x"),
      element.GetAttributeFloat("y")
    );
    object.size = sf::Vector2f(
      element.GetAttributeFloat("width"),
      element.GetAttributeFloat("height")
    );
    object.rotation = element.GetAttributeFloat("rotation");
    object.visible = element.GetAttribute("visible") != "0";

    for (auto child : element.children) {
      if (child.name == "properties") {
        object.customProperties = CustomProperties::From(child);
      }
    }
  }

  std::optional<ShapeObject> ShapeObject::From(const XMLElement& element) {
    auto id = element.GetAttributeInt("id");
    auto shapePtr = Shape::From(element);

    if (!shapePtr) {
      return {};
    }

    auto shapeObject = ShapeObject(id, std::move(shapePtr.value()));

    AdoptAttributes(shapeObject, element);

    return shapeObject;
  }

  TileObject TileObject::From(const XMLElement& element) {
    auto id = element.GetAttributeInt("id");
    auto gid = static_cast<unsigned int>(stoul("0" + element.GetAttribute("gid")));

    auto tileObject = TileObject(id, gid);

    AdoptAttributes(tileObject, element);

    return tileObject;
  }
}
