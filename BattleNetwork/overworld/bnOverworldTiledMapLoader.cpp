#include "bnOverworldTiledMapLoader.h"

#include "bnXML.h"

namespace Overworld {
  static std::shared_ptr<Tileset> ParseTileset(SceneBase& scene, const XMLElement& tilesetElement, unsigned int firstgid) {
    auto tileCount = static_cast<unsigned int>(tilesetElement.GetAttributeInt("tilecount"));
    auto tileWidth = tilesetElement.GetAttributeInt("tilewidth");
    auto tileHeight = tilesetElement.GetAttributeInt("tileheight");
    auto columns = tilesetElement.GetAttributeInt("columns");

    sf::Vector2f drawingOffset;
    std::string texturePath;
    Projection orientation = Projection::Orthographic;
    CustomProperties customProperties;

    std::vector<XMLElement> tileElements(tileCount);

    for (auto& child : tilesetElement.children) {
      if (child.name == "tileoffset") {
        drawingOffset.x = child.GetAttributeFloat("x");
        drawingOffset.y = child.GetAttributeFloat("y");
      }
      else if (child.name == "grid") {
        orientation =
          child.GetAttribute("orientation") == "isometric" ?
          Projection::Isometric
          : Projection::Orthographic;
      }
      else if (child.name == "image") {
        texturePath = child.GetAttribute("source");

        if (texturePath.find("/server", 0) != 0) {
          // client path
          // hardcoded
          texturePath = "resources/ow/tiles/" + texturePath;
        }
      }
      else if (child.name == "tile") {
        auto tileId = child.GetAttributeInt("id");

        if (tileElements.size() <= tileId) {
          size_t sz = static_cast<size_t>(tileId) + 1;
          tileElements.resize(sz);
        }

        tileElements[tileId] = child;
      }
      else if (child.name == "properties") {
        customProperties = CustomProperties::From(child);
      }
    }

    // todo: work with Animation class directly?
    std::string animationString = "imagePath=\"./" + texturePath + "\"\n\n";

    auto objectAlignment = tilesetElement.GetAttribute("objectalignment");
    // default to bottom
    auto alignmentOffset = sf::Vector2i(-tileWidth / 2, -tileHeight);

    if (objectAlignment == "top") {
      alignmentOffset = sf::Vector2i(-tileWidth / 2, 0);
    }
    else if (objectAlignment == "topleft") {
      alignmentOffset = sf::Vector2i(0, 0);
    }
    else if (objectAlignment == "topright") {
      alignmentOffset = sf::Vector2i(-tileWidth, 0);
    }
    else if (objectAlignment == "center") {
      alignmentOffset = sf::Vector2i(-tileWidth / 2, -tileHeight / 2);
    }
    else if (objectAlignment == "left") {
      alignmentOffset = sf::Vector2i(0, -tileHeight / 2);
    }
    else if (objectAlignment == "right") {
      alignmentOffset = sf::Vector2i(-tileWidth, -tileHeight / 2);
    }
    else if (objectAlignment == "bottomleft") {
      alignmentOffset = sf::Vector2i(0, -tileHeight);
    }
    else if (objectAlignment == "bottomright") {
      alignmentOffset = sf::Vector2i(-tileWidth, -tileHeight);
    }

    std::string frameOffsetString = " originx=\"" + to_string(tileWidth / 2) + "\" originy=\"" + to_string(tileHeight / 2) + '"';
    std::string frameSizeString = " w=\"" + to_string(tileWidth) + "\" h=\"" + to_string(tileHeight) + '"';

    for (auto i = 0; i < tileElements.size(); i++) {
      auto& tileElement = tileElements[i];
      animationString += "animation state=\"" + to_string(i) + "\"\n";

      bool foundAnimation = false;

      for (auto& child : tileElement.children) {
        if (child.name == "animation") {
          for (auto& frameElement : child.children) {
            if (frameElement.name != "frame") {
              continue;
            }

            foundAnimation = true;

            auto tileId = frameElement.GetAttributeInt("tileid");
            auto duration = frameElement.GetAttributeInt("duration") / 1000.0;

            auto col = tileId % columns;
            auto row = tileId / columns;

            animationString += "frame duration=\"" + to_string(duration) + '"';
            animationString += " x=\"" + to_string(col * tileWidth) + "\" y=\"" + to_string(row * tileHeight) + '"';
            animationString += frameSizeString + frameOffsetString + '\n';
          }
        }
      }

      if (!foundAnimation) {
        auto col = i % columns;
        auto row = i / columns;

        animationString += "frame duration=\"0\"";
        animationString += " x=\"" + to_string(col * tileWidth) + "\" y=\"" + to_string(row * tileHeight) + '"';
        animationString += frameSizeString + frameOffsetString + '\n';
      }

      animationString += "\n";
    }

    Animation animation;
    animation.LoadWithData(animationString);

    auto tileset = Tileset{
      tilesetElement.GetAttribute("name"),
      firstgid,
      tileCount,
      drawingOffset,
      sf::Vector2f(alignmentOffset),
      orientation,
      customProperties,
      scene.GetTexture(texturePath),
      animation
    };

    return std::make_shared<Tileset>(tileset);
  }

  static std::vector<std::shared_ptr<TileMeta>>
    ParseTileMetas(const XMLElement& tilesetElement, const Tileset& tileset) {
    auto tileCount = static_cast<unsigned int>(tilesetElement.GetAttributeInt("tilecount"));

    std::vector<XMLElement> tileElements(tileCount);

    for (auto& child : tilesetElement.children) {
      if (child.name == "tile") {
        auto tileId = child.GetAttributeInt("id");

        if (tileElements.size() <= tileId) {
          size_t sz = static_cast<size_t>(tileId) + 1;
          tileElements.resize(sz);
        }

        tileElements[tileId] = child;
      }
    }

    std::vector<std::shared_ptr<TileMeta>> tileMetas;
    auto tileId = 0;
    auto tileGid = tileset.firstGid;

    for (auto& tileElement : tileElements) {
      std::vector<std::unique_ptr<Shape>> collisionShapes;
      CustomProperties customProperties;

      for (auto& child : tileElement.children) {
        if (child.name == "properties") {
          customProperties = CustomProperties::From(child);
          continue;
        }

        if (child.name != "objectgroup") {
          continue;
        }

        for (auto& objectElement : child.children) {
          auto shape = Shape::From(objectElement);

          if (shape) {
            collisionShapes.push_back(std::move(*shape));
          }
        }
      }

      auto tileMeta = std::make_shared<TileMeta>(
        tileset,
        tileId,
        tileGid,
        tileset.drawingOffset,
        tileset.alignmentOffset,
        TileType::FromString(tileElement.GetAttribute("type")),
        TileShadow::FromString(customProperties.GetProperty("shadow")),
        customProperties,
        std::move(collisionShapes)
        );

      tileMetas.push_back(tileMeta);
      tileId += 1;
      tileGid += 1;
    }

    return tileMetas;
  }

  std::optional<Map> LoadTiledMap(SceneBase& scene, const std::string& data)
  {
    XMLElement mapElement = parseXML(data);

    if (mapElement.name != "map") {
      return {};
    }

    // organize elements
    XMLElement propertiesElement;
    std::vector<XMLElement> layerElements;
    std::vector<XMLElement> objectLayerElements;
    std::vector<XMLElement> tilesetElements;

    for (auto& child : mapElement.children) {
      if (child.name == "layer") {
        layerElements.push_back(child);
      }
      else if (child.name == "objectgroup") {
        objectLayerElements.push_back(child);
      }
      else if (child.name == "properties") {
        propertiesElement = child;
      }
      else if (child.name == "tileset") {
        tilesetElements.push_back(child);
      }
    }

    auto tileWidth = mapElement.GetAttributeInt("tilewidth");
    auto tileHeight = mapElement.GetAttributeInt("tileheight");

    // begin building map
    auto map = Map(
      mapElement.GetAttributeInt("width"),
      mapElement.GetAttributeInt("height"),
      tileWidth,
      tileHeight);

    // read custom properties
    for (const auto& propertyElement : propertiesElement.children) {
      auto propertyName = propertyElement.GetAttribute("name");
      auto propertyValue = propertyElement.GetAttribute("value");

      if (propertyName == "Background") {
        std::transform(propertyValue.begin(), propertyValue.end(), propertyValue.begin(), [](auto in) {
          return std::tolower(in);
        });

        map.SetBackgroundName(propertyValue);
      }
      else if (propertyName == "Background Texture") {
        map.SetBackgroundCustomTexturePath(propertyValue);
      }
      else if (propertyName == "Background Animation") {
        map.SetBackgroundCustomAnimationPath(propertyValue);
      }
      else if (propertyName == "Background Vel X") {
        auto velocity = map.GetBackgroundCustomVelocity();
        velocity.x = propertyElement.GetAttributeFloat("value");

        map.SetBackgroundCustomVelocity(velocity);
      }
      else if (propertyName == "Background Vel Y") {
        auto velocity = map.GetBackgroundCustomVelocity();
        velocity.y = propertyElement.GetAttributeFloat("value");

        map.SetBackgroundCustomVelocity(velocity);
      }
      else if (propertyName == "Background Parallax") {
        map.SetBackgroundParallax(propertyElement.GetAttributeFloat("value"));
      }
      else if (propertyName == "Foreground Texture") {
        map.SetForegroundTexturePath(propertyValue);
      }
      else if (propertyName == "Foreground Animation") {
        map.SetForegroundAnimationPath(propertyValue);
      }
      else if (propertyName == "Foreground Vel X") {
        auto velocity = map.GetForegroundVelocity();
        velocity.x = propertyElement.GetAttributeFloat("value");

        map.SetForegroundVelocity(velocity);
      }
      else if (propertyName == "Foreground Vel Y") {
        auto velocity = map.GetForegroundVelocity();
        velocity.y = propertyElement.GetAttributeFloat("value");

        map.SetForegroundVelocity(velocity);
      }
      else if (propertyName == "Foreground Parallax") {
        map.SetForegroundParallax(propertyElement.GetAttributeFloat("value"));
      }
      else if (propertyName == "Name") {
        map.SetName(propertyValue);
      }
      else if (propertyName == "Song") {
        map.SetSongPath(propertyValue);
      }
    }

    // load tilesets
    for (auto& mapTilesetElement : tilesetElements) {
      auto firstgid = static_cast<unsigned int>(mapTilesetElement.GetAttributeInt("firstgid"));
      auto source = mapTilesetElement.GetAttribute("source");

      if (source.find("/server", 0) != 0) {
        // client path
        // todo: hardcoded path oof, this will only be fine if all of our tiles are in this folder
        size_t pos = source.rfind('/');

        if (pos != std::string::npos) {
          source = "resources/ow/tiles" + source.substr(pos);
        }
      }

      XMLElement tilesetElement = parseXML(scene.GetText(source));
      auto tileset = ParseTileset(scene, tilesetElement, firstgid);
      auto tileMetas = ParseTileMetas(tilesetElement, *tileset);

      for (auto& tileMeta : tileMetas) {
        map.SetTileset(tileset, tileMeta);
      }
    }

    int layerCount = (int)std::max(layerElements.size(), objectLayerElements.size());

    // build layers
    for (int i = 0; i < layerCount; i++) {
      auto& layer = map.AddLayer();

      // add tiles to layer
      if (layerElements.size() > i) {
        auto& layerElement = layerElements[i];

        layer.SetVisible(layerElement.attributes["visible"] != "0");

        auto dataIt = std::find_if(layerElement.children.begin(), layerElement.children.end(), [](XMLElement& el) {return el.name == "data";});
        if (dataIt == layerElement.children.end()) {
          Logger::Log(LogLevel::warning, "Map layer missing data element!");
          continue;
        }

        auto& dataElement = *dataIt;

        auto dataLen = dataElement.text.length();

        auto col = 0;
        auto row = 0;
        auto sliceStart = 0;

        for (size_t i = 0; i <= dataLen; i++) {
          switch (dataElement.text[i]) {
          case '\0':
          case ',': {
            auto tileId = static_cast<unsigned int>(stoul("0" + dataElement.text.substr(sliceStart, i - sliceStart)));

            layer.SetTile(col, row, tileId);

            sliceStart = static_cast<int>(i) + 1;
            col++;
            break;
          }
          case '\n':
            sliceStart = static_cast<int>(i) + 1;
            col = 0;
            row++;
            break;
          default:
            break;
          }
        }
      }

      // add objects to layer
      if (objectLayerElements.size() > i) {
        auto& objectLayerElement = objectLayerElements[i];

        for (auto& child : objectLayerElement.children) {
          if (child.name != "object") {
            continue;
          }

          if (child.HasAttribute("gid")) {
            auto tileObject = TileObject::From(child);
            auto tilePosition = map.WorldToTileSpace(tileObject.position);
            auto elevation = map.GetElevationAt(tilePosition.x, tilePosition.y, i);

            tileObject.GetWorldSprite()->SetElevation(elevation);
            layer.AddTileObject(tileObject);
          }
          else {
            auto shapeObject = ShapeObject::From(child);

            if (shapeObject) {
              layer.AddShapeObject(std::move(*shapeObject));
            }
          }
        }
      }
    }

    return std::move(map);
  }
}