#include <algorithm>
#include <cmath>

#include "bnOverworldMap.h"
#include "bnOverworldSceneBase.h"


namespace Overworld {
  Map::Map(unsigned cols, unsigned rows, int tileWidth, int tileHeight) :
    shadowMap(cols, rows)
  {
    this->tileWidth = tileWidth;
    this->tileHeight = tileHeight;
    this->cols = cols;
    this->rows = rows;
    this->tileToTilesetMap.push_back(nullptr);
    this->tileMetas.resize(1);
  }

  void Map::Update(SceneBase& scene, double time) {
    for (auto& tileMeta : tileMetas) {
      if (tileMeta != nullptr) {
        tileMeta->animation.SyncTime(from_seconds(time));
        tileMeta->animation.Refresh(tileMeta->sprite);
      }
    }

    for (int i = 0; i < layers.size(); i++) {
      auto& layer = layers[i];

      tilesModified |= layer.tilesModified;
      layer.tilesModified = false;

      for (auto& tileObject : layer.tileObjects) {
        tileObject.Update(*this);
      }

      for (auto& worldSprite : layer.spritesForAddition) {
        scene.AddSprite(worldSprite);
      }

      layer.spritesForAddition.clear();
    }

    if (tilesModified) {
      shadowMap.CalculateShadows(*this);
      tilesModified = false;
    }
  }

  sf::Vector2f Map::ScreenToWorld(sf::Vector2f screen) const {
    sf::Vector2f world{};
    world.x = (2.0f * screen.y + screen.x) * 0.5f;
    world.y = (2.0f * screen.y - screen.x) * 0.5f;

    return world;
  }

  sf::Vector2f Map::WorldToScreen(sf::Vector2f world) const {
    sf::Vector2f screen{};
    screen.x = (world.x - world.y);
    screen.y = (world.x + world.y) * 0.5f;

    return screen;
  }

  sf::Vector2f Map::WorldToScreen(sf::Vector3f world) const
  {
    auto screenPos = WorldToScreen({ world.x, world.y });
    screenPos.y -= world.z * tileHeight / 2.0f;

    return screenPos;
  }

  sf::Vector2f Map::WorldToTileSpace(sf::Vector2f world) const
  {
    return { world.x / (float)tileWidth * 2.0f, world.y / (float)tileHeight };
  }

  sf::Vector2f Map::TileToWorld(sf::Vector2f tileSpace) const
  {
    return { tileSpace.x * (float)tileWidth * 0.5f, tileSpace.y * (float)tileHeight };
  }

  Tile* Map::GetTileFromWorld(sf::Vector3f worldPos) {
    if(worldPos.z < 0) {
      return nullptr;
    }

    auto layerIndex = size_t(worldPos.z);
    auto tilePos = sf::Vector2i(WorldToTileSpace({ worldPos.x, worldPos.y }));

    if (layerIndex >= layers.size()) {
      return nullptr;
    }

    auto& layer = layers[layerIndex];

    return layer.GetTile(tilePos.x, tilePos.y);
  }

  size_t Map::HashTilePosition(sf::Vector2f position) const {
    return size_t(position.x) + size_t(cols) * size_t(position.y);
  }

  sf::Vector2i Map::GetTileSize() const {
    return sf::Vector2i(tileWidth, tileHeight);
  }

  const std::string& Map::GetName() const
  {
    return name;
  }

  const std::string& Map::GetBackgroundName() const
  {
    return backgroundName;
  }

  const std::string& Map::GetBackgroundCustomTexturePath() const
  {
    return backgroundCustomTexturePath;
  }

  const std::string& Map::GetBackgroundCustomAnimationPath() const
  {
    return backgroundCustomAnimationPath;
  }

  sf::Vector2f Map::GetBackgroundCustomVelocity() const
  {
    return backgroundCustomVelocity;
  }

  float Map::GetBackgroundParallax() const
  {
    return backgroundParallax;
  }

  const std::string& Map::GetForegroundTexturePath() const
  {
    return foregroundTexturePath;
  }

  const std::string& Map::GetForegroundAnimationPath() const
  {
    return foregroundAnimationPath;
  }

  sf::Vector2f Map::GetForegroundVelocity() const
  {
    return foregroundVelocity;
  }

  float Map::GetForegroundParallax() const
  {
    return foregroundParallax;
  }

  const std::string& Map::GetSongPath() const
  {
    return songPath;
  }

  void Map::SetName(const std::string& name)
  {
    this->name = name;
  }

  void Map::SetBackgroundName(const std::string& name)
  {
    backgroundName = name;
  }

  void Map::SetBackgroundCustomTexturePath(const std::string& path) {
    backgroundCustomTexturePath = path;
  }

  void Map::SetBackgroundCustomAnimationPath(const std::string& path) {
    backgroundCustomAnimationPath = path;
  }

  void Map::SetBackgroundCustomVelocity(float x, float y) {
    backgroundCustomVelocity.x = x;
    backgroundCustomVelocity.y = y;
  }

  void Map::SetBackgroundCustomVelocity(sf::Vector2f velocity) {
    backgroundCustomVelocity = velocity;
  }

  void Map::SetBackgroundParallax(float factor) {
    backgroundParallax = factor;
  }

  void Map::SetForegroundTexturePath(const std::string& path) {
    foregroundTexturePath = path;
  }

  void Map::SetForegroundAnimationPath(const std::string& path) {
    foregroundAnimationPath = path;
  }

  void Map::SetForegroundVelocity(float x, float y) {
    foregroundVelocity.x = x;
    foregroundVelocity.y = y;
  }

  void Map::SetForegroundVelocity(sf::Vector2f velocity) {
    foregroundVelocity = velocity;
  }

  void Map::SetForegroundParallax(float factor) {
    foregroundParallax = factor;
  }

  void Map::SetSongPath(const std::string& path)
  {
    songPath = path;
  }

  unsigned Map::GetCols() const
  {
    return cols;
  }

  unsigned Map::GetRows() const
  {
    return rows;
  }

  unsigned int Map::GetTileCount() {
    return static_cast<unsigned int>(tileMetas.size());
  }

  std::shared_ptr<TileMeta>& Map::GetTileMeta(unsigned int tileGid) {
    if (tileGid < 0 || tileGid >= tileMetas.size()) {
      return tileMetas[0];
    }

    return tileMetas[tileGid];
  }

  std::shared_ptr<Tileset> Map::GetTileset(const std::string& name) {
    if (tilesets.find(name) == tilesets.end()) {
      return nullptr;
    }

    return tilesets[name];
  }

  std::shared_ptr<Tileset> Map::GetTileset(unsigned int tileGid) {
    if (tileToTilesetMap.size() <= tileGid) {
      return nullptr;
    }

    return tileToTilesetMap[tileGid];
  }

  void Map::SetTileset(const std::shared_ptr<Tileset>& tileset, const std::shared_ptr<TileMeta>& tileMeta) {
    auto tileGid = tileMeta->gid;

    if (tileToTilesetMap.size() <= tileGid) {
      tileToTilesetMap.resize(size_t(tileGid + 1u));
      tileMetas.resize(size_t(tileGid + 1u));
    }

    tileMetas[tileGid] = tileMeta;
    tileToTilesetMap[tileGid] = tileset;
    tilesets[tileset->name] = tileset;
    tilesModified = true;
  }

  std::size_t Map::GetLayerCount() const {
    return layers.size();
  }

  Map::Layer& Map::GetLayer(size_t index) {
    return layers.at(index);
  }

  Map::Layer& Map::AddLayer() {
    layers.push_back(std::move(Layer(cols, rows)));

    return layers[layers.size() - 1];
  }

  // todo: move to layer?
  // may require reference to map as tilemeta + tile size is used
  bool Map::CanMoveTo(float x, float y, float z, int layerIndex) {
    if (layerIndex < 0 || layerIndex >= layers.size()) {
      return false;
    }

    auto& layer = GetLayer(layerIndex);
    auto tile = layer.GetTile(x, y);

    if (!tile) {
      return false;
    }

    // get decimal part
    float tileX{};
    float tileY{};
    auto testPosition = sf::Vector2f(
      std::modf(x, &tileX),
      std::modf(y, &tileY)
    );

    // get positive coords
    if (testPosition.x < 0) {
      testPosition.x += 1;
    }
    if (testPosition.y < 0) {
      testPosition.y += 1;
    }

    // convert to iso pixels
    testPosition.x *= tileWidth / 2;
    testPosition.y *= tileHeight;

    float layerElevation;
    auto layerRelativeZ = std::modf(z, &layerElevation);
    auto tileTestPos = sf::Vector2f(
      testPosition.x - layerRelativeZ * tileHeight,
      testPosition.y - layerRelativeZ * tileHeight
    );

    if (tile->Intersects(*this, tileTestPos.x, tileTestPos.y)) {
      return false;
    }

    testPosition.x += tileX * tileWidth / 2;
    testPosition.y += tileY * tileHeight;

    // todo: use a spatial map for increased performance
    for (auto& tileObject : layer.GetTileObjects()) {
      if (tileObject.solid && tileObject.Intersects(*this, testPosition.x, testPosition.y)) {
        return false;
      }
    }

    return true;
  }

  float Map::GetElevationAt(float x, float y, int layerIndex) {
    auto totalLayers = layers.size();

    if (layerIndex >= totalLayers) {
      layerIndex = (int)totalLayers - 1;
    }

    layerIndex = std::max(layerIndex, 0);
    auto layerElevation = (float)layerIndex;

    auto& layer = layers[layerIndex];
    auto tile = layer.GetTile(x, y);

    if (!tile) {
      return layerElevation;
    }

    auto& tileMeta = GetTileMeta(tile->gid);

    if (!tileMeta || tileMeta->type != TileType::stairs) {
      return layerElevation;
    }

    float _, relativeX, relativeY;
    relativeX = std::modf(x, &_);
    relativeY = std::modf(y, &_);

    float layerRelativeElevation = 0.0;
    Direction direction = FromString(tileMeta->customProperties.GetProperty("direction"));

    if (tile->flippedHorizontal) {
      direction = FlipHorizontal(direction);
    }

    if (tile->flippedVertical) {
      direction = FlipVertical(direction);
    }

    switch (direction) {
    case Direction::up_left:
      layerRelativeElevation = 1 - relativeX;
      break;
    case Direction::up_right:
      layerRelativeElevation = 1 - relativeY;
      break;
    case Direction::down_left:
      layerRelativeElevation = relativeY;
      break;
    case Direction::down_right:
      layerRelativeElevation = relativeX;
      break;
    default:
      break;
    }

    return layerElevation + layerRelativeElevation;
  }

  bool Map::IgnoreTileAbove(float x, float y, int layerIndex) {
    if (layerIndex < 0 || layerIndex >= layers.size()) {
      return false;
    }

    auto& layer = layers[layerIndex];
    auto tile = layer.GetTile(x, y);

    if (!tile) {
      return false;
    }

    auto& tileMeta = GetTileMeta(tile->gid);

    return tileMeta && tileMeta->type == TileType::stairs;
  }

  bool Map::HasShadow(sf::Vector2i tilePos, int layer) {
    if (tilePos.x < 0 || tilePos.y < 0 || layer < 0) {
      return false;
    }

    return shadowMap.HasShadow(tilePos.x, tilePos.y, layer);
  }

  bool Map::TileConceals(sf::Vector2i tilePos, int layer) {
    if (layer < 0 || layer >= layers.size()) {
      return false;
    }

    auto tile = layers[layer].GetTile(tilePos.x, tilePos.y);

    if (!tile) {
      return false;
    }

    auto tileMeta = GetTileMeta(tile->gid);

    return tileMeta && tileMeta->type != TileType::invisible && !IgnoreTileAbove(tilePos.x, tilePos.y, layer - 1);
  }

  bool Map::IsConcealed(sf::Vector2i tilePos, int layer) {
    auto col = tilePos.x;
    auto row = tilePos.y;

    for (auto i = layer + 1; i < layers.size(); i++) {
      auto layerOffset = i - layer;
      auto isOddLayer = layerOffset % 2 == 1;

      if (!isOddLayer) {
        // every two layers we move, we have a new tile that aligns with us
        col += 1;
        row += 1;
      }

      if (col < 0 || row < 0 || col >= cols || row >= rows || i < 0) {
        continue;
      }

      if (TileConceals({ col, row }, i)) {
        return true;
      }

      if (isOddLayer && TileConceals({ col + 1, row + 1 }, i)) {
        return true;
      }
    }

    return false;
  }

  void Map::RemoveSprites(SceneBase& scene) {
    for (auto& layer : layers) {
      for (auto& tileObject : layer.tileObjects) {
        auto spriteProxy = tileObject.GetWorldSprite();

        if (spriteProxy) {
          scene.RemoveSprite(spriteProxy);
        }
      }
    }
  }

  Map::Layer::Layer(unsigned cols, unsigned rows) {
    this->cols = cols;
    this->rows = rows;
    tiles.resize(cols * rows, Tile(0));
  }

  Tile* Map::Layer::GetTile(int x, int y)
  {
    if (x < 0 || y < 0 || x >= (int)cols || y >= (int)rows) {
      return nullptr;
    }

    return &tiles[y * cols + x];
  }

  Tile* Map::Layer::GetTile(float x, float y)
  {
    return GetTile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
  }

  Tile* Map::Layer::SetTile(int x, int y, Tile tile)
  {
    auto storedTile = GetTile(x, y);

    if (storedTile) {
      *storedTile = tile;
    }

    return storedTile;
  }

  Tile* Map::Layer::SetTile(int x, int y, unsigned int gid)
  {
    return SetTile(x, y, Tile(gid));
  }

  Tile* Map::Layer::SetTile(float x, float y, unsigned int gid)
  {
    return SetTile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)), gid);
  }

  void Map::Layer::SetVisible(bool enabled)
  {
    visible = enabled;
  }

  bool Map::Layer::IsVisible() const
  {
    return visible;
  }

  std::optional<std::reference_wrapper<TileObject>> Map::Layer::GetTileObject(unsigned int id) {
    auto iterEnd = tileObjects.end();
    auto iter = std::find_if(tileObjects.begin(), tileObjects.end(), [id](TileObject& tileObject) { return tileObject.id == id; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  std::optional<std::reference_wrapper<TileObject>> Map::Layer::GetTileObject(const std::string& name) {
    auto iterEnd = tileObjects.end();
    auto iter = std::find_if(tileObjects.begin(), tileObjects.end(), [name](TileObject& tileObject) { return tileObject.name == name; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  std::vector<TileObject>& Map::Layer::GetTileObjects() {
    return tileObjects;
  }

  void Map::Layer::AddTileObject(TileObject tileObject) {
    tileObjects.push_back(tileObject);
    spritesForAddition.push_back(tileObject.GetWorldSprite());
  }

  std::optional<std::reference_wrapper<ShapeObject>> Map::Layer::GetShapeObject(unsigned int id) {
    auto iterEnd = shapeObjects.end();
    auto iter = std::find_if(shapeObjects.begin(), shapeObjects.end(), [id](ShapeObject& shapeObject) { return shapeObject.id == id; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  std::optional<std::reference_wrapper<ShapeObject>> Map::Layer::GetShapeObject(const std::string& name) {
    auto iterEnd = shapeObjects.end();
    auto iter = std::find_if(shapeObjects.begin(), shapeObjects.end(), [name](ShapeObject& shapeObject) { return shapeObject.name == name; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  const std::vector<ShapeObject>& Map::Layer::GetShapeObjects() {
    return shapeObjects;
  }

  void Map::Layer::AddShapeObject(ShapeObject shapeObject) {
    shapeObjects.push_back(std::move(shapeObject));
  }
}
