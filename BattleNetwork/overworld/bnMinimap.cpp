#include "bnMinimap.h"

#include "bnOverworldTileType.h"
#include "../bnTextureResourceManager.h"
#include "../bnShaderResourceManager.h"
#include "../bnMath.h"
#include "../stx/string.h"

#include <SFML/Graphics.hpp>
#include <Swoosh/EmbedGLSL.h>
#include <cmath>

const auto PLAYER_COLOR = sf::Color(0, 248, 248, 255);
const auto CONCEALED_COLOR = sf::Color(165, 165, 165, 165); // 65% transparency, similar to world sprite darkening

static void CopyFrame(SpriteProxyNode& dest, const SpriteProxyNode& source) {
  dest.setTexture(source.getTexture());
  dest.setTextureRect(source.getTextureRect());
  dest.getSprite().setOrigin(source.getSprite().getOrigin());
}

const float MAX_TILE_HEIGHT = 10.f;
const float MIN_TILE_HEIGHT = 4.f;

static float resolveTileHeight(Overworld::Map& map, sf::Vector2f mapScreenUnitDimensions) {
  auto targetTileHeight = std::min(160.f / (mapScreenUnitDimensions.y), 240.f / (mapScreenUnitDimensions.x * 2.0f) * 240.0f / 160.0f) * 2.0f;
  targetTileHeight = std::clamp(targetTileHeight, MIN_TILE_HEIGHT, MAX_TILE_HEIGHT);

  if (targetTileHeight <= MIN_TILE_HEIGHT) {
    targetTileHeight = MIN_TILE_HEIGHT;
  }

  return targetTileHeight;
}

void Overworld::Minimap::Update(const std::string& name, Map& map)
{
  this->name = name;

  // prepare a render texture to write to
  sf::RenderTexture texture;
  sf::RenderStates states;

  const auto screenSize = sf::Vector2i{ 240, 160 };
  auto mapScreenUnitDimensions = sf::Vector2f(
    map.WorldToScreen({ (float)map.GetCols(), 0.0f }).x - map.WorldToScreen({ 0.0f, (float)map.GetRows() }).x,
    float(map.GetCols() + map.GetRows()) + float(map.GetLayerCount()) + 1.0f
  );

  auto tileSize = map.GetTileSize();
  auto tileHeight = resolveTileHeight(map, mapScreenUnitDimensions);

  scaling = tileHeight / tileSize.y;

  // how many times could this map fit into the screen? make that many.
  sf::Vector2f texsizef = {
    std::ceil(mapScreenUnitDimensions.x * tileSize.x * scaling * 0.5f),
    std::ceil(mapScreenUnitDimensions.y * tileSize.y * scaling * 0.5f) + 1.0f
  };

  auto textureSize = sf::Vector2i(texsizef);

  textureSize.x = std::max(screenSize.x, textureSize.x);
  textureSize.y = std::max(screenSize.y, textureSize.y);

  // texture does not fit on screen, allow large map controls
  largeMapControls = textureSize.x > screenSize.x || textureSize.y > screenSize.y;

  if (!texture.create(textureSize.x, textureSize.y)) return;

  // fill with background color
  texture.clear(sf::Color(0,0,0,0));

  /**
  layer1= 152 144 224
  layer2= 176 168 240 diff = 24 24 16
  layer3= 208 200 248 diff = 24 32  8
                             ---------
                             0  +8 -8

  potential layer4= 208, 208, 240? ??
  **/

  sf::Color layer1Color = sf::Color(152, 144, 224);
  sf::Color layerMaxColor = sf::Color(208, 208, 240);

  sf::Color edge1Color = sf::Color(120, 112, 192);
  sf::Color edgeMaxColor = sf::Color(160, 152, 224); // sf::Color(168, 160, 240);

    // shader pass transforms opaque pixels into purple hues
  static ResourceHandle handle;
  states.shader = handle.Shaders().GetShader(ShaderType::MINIMAP_COLOR);

  // guestimate best fit "center" of the map
  auto layerDimensions = map.TileToWorld({ (float)map.GetCols(), (float)map.GetRows() });
  sf::Vector3f mapDimensions = {
    layerDimensions.x, layerDimensions.y,
    (float)map.GetLayerCount() - 1.0f
  };

  // move the map to the center of the screen and fit
  // TODO: chunk the minimap for large map
  sf::Vector2f center = map.WorldToScreen(sf::Vector3f(mapDimensions. x, mapDimensions.y, mapDimensions.z * 2.0f) * 0.5f) * scaling;
  offset = center;

  const int maxLayerCount = (int)map.GetLayerCount();

  // apply transforms
  states.transform.translate((240.f * 0.5f) - center.x, (160.f * 0.5f) - center.y);
  states.transform.scale(scaling, scaling);

  // draw. every layer passes through the shader
  for (auto i = 0; i < maxLayerCount; i++) {
    auto lerpColor = [](int index, int maxCount, sf::Color minColor, sf::Color maxColor) {
      if (index > maxCount) {
        index = maxCount;
      }

      float delta = ((float)index / (float)maxCount);

      float r = (1.f - delta) * minColor.r + (delta * maxColor.r);
      float g = (1.f - delta) * minColor.g + (delta * maxColor.g);
      float b = (1.f - delta) * minColor.b + (delta * maxColor.b);

      return sf::Glsl::Vec4(r / 255.f, g / 255.f, b / 255.f, 1.f);
    };

    states.shader->setUniform("finalColor", lerpColor(i, maxLayerCount, layer1Color, layerMaxColor));

    // draw layer (Applies masking)
    DrawLayer(texture, *states.shader, states, map, i);

    // iso layers are offset (prepare for the next layer)
    states.transform.translate(0.f, -tileSize.y * 0.5f);
  }

  // Grab image data to make a texture
    // NOTE: gpu <-> cpu is a costly process. do this sparringly.
  texture.display();
  sf::Texture tempTex = texture.getTexture();
  sf::Sprite temp(tempTex);

  // do a second pass for edge detection
  states.shader = handle.Shaders().GetShader(ShaderType::MINIMAP_EDGE);
  states.transform = sf::Transform::Identity;
  states.shader->setUniform("resolutionW", (float)textureSize.x);
  states.shader->setUniform("resolutionH", (float)textureSize.y);
  texture.draw(temp, states);

  texture.display();

  // set the final texture
  bakedMap.setTexture(std::make_shared<sf::Texture>(texture.getTexture()));

  FindMapMarkers(map);
}

void Overworld::Minimap::FindMapMarkers(Map& map) {
  // remove old map markers
  for(auto& marker : mapMarkers) {
    bakedMap.RemoveNode(marker.get());
  }

  mapMarkers.clear();

  // add new markers
  FindTileMarkers(map);
  FindObjectMarkers(map);
}

void Overworld::Minimap::FindTileMarkers(Map& map) {
  auto layerCount = map.GetLayerCount();
  auto cols = map.GetCols();
  auto rows = map.GetRows();

  for (auto i = 0; i < layerCount; i++) {
    auto& layer = map.GetLayer(i);

    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        Tile* tile = layer.GetTile((int)col, (int)row);
        std::shared_ptr<Overworld::TileMeta>& tileMeta = map.GetTileMeta(tile->gid);

        if (!tileMeta) {
          continue;
        }

        switch (tileMeta->type) {
        case TileType::conveyor: {
          auto pos = sf::Vector2f(col, row);
          pos.x += 0.5f;
          pos.y += 0.5f;

          auto worldPos = map.TileToWorld(pos);
          auto screenPos = map.WorldToScreen({ worldPos.x, worldPos.y, float(i) });

          bool isConcealed = map.IsConcealed(sf::Vector2i(col, row), i);
          auto direction = tile->GetDirection(*tileMeta);
          AddConveyorPosition(screenPos, direction, isConcealed);
          break;
        }
        case TileType::arrow:{
          auto pos = sf::Vector2f(col, row);
          pos.x += 0.5f;
          pos.y += 0.5f;

          auto direction = tile->GetDirection(*tileMeta);
          pos += Round(UnitVector(Orthographic(direction))) * 0.5f;

          auto worldPos = map.TileToWorld(pos);
          auto screenPos = map.WorldToScreen({ worldPos.x, worldPos.y, float(i) });

          AddArrowPosition(screenPos, direction);
          break;
        }
        default:
          break;
        }
      }
    }
  }
}

void Overworld::Minimap::FindObjectMarkers(Map& map) {
  auto layerCount = map.GetLayerCount();
  auto tileSize = map.GetTileSize();

  for (auto i = 0; i < layerCount; i++) {
    for (auto& tileObject : map.GetLayer(i).GetTileObjects()) {
      auto tileMeta = map.GetTileMeta(tileObject.tile.gid);

      if (!tileMeta) continue;

      auto screenOffset = tileMeta->alignmentOffset + tileMeta->drawingOffset;
      screenOffset += tileObject.size / 2.0f;

      auto objectCenterPos = tileObject.position + map.ScreenToWorld(screenOffset);
      auto zOffset = sf::Vector2f(0, (float)(-i * tileSize.y / 2));

      if (tileObject.type == ObjectType::home_warp) {
        bool isConcealed = map.IsConcealed(sf::Vector2i(map.WorldToTileSpace(objectCenterPos)), i);
        AddHomepagePosition(map.WorldToScreen(objectCenterPos) + zOffset, isConcealed);
      }
      else if (ObjectType::IsWarp(tileObject.type)) {
        bool isConcealed = map.IsConcealed(sf::Vector2i(map.WorldToTileSpace(objectCenterPos)), i);
        AddWarpPosition(map.WorldToScreen(objectCenterPos) + zOffset, isConcealed);
      }
      else if (tileObject.type == ObjectType::board) {
        sf::Vector2f bottomPosition = objectCenterPos;
        bottomPosition += map.ScreenToWorld({ 0.0f, tileObject.size.y / 2.0f });

        bool isConcealed = map.IsConcealed(sf::Vector2i(map.WorldToTileSpace(bottomPosition)), i);
        AddBoardPosition(map.WorldToScreen(bottomPosition) + zOffset, tileObject.tile.flippedHorizontal, isConcealed);
      }
      else if (tileObject.type == ObjectType::shop) {
        bool isConcealed = map.IsConcealed(sf::Vector2i(map.WorldToTileSpace(tileObject.position)), i);
        AddShopPosition(map.WorldToScreen(tileObject.position) + zOffset, isConcealed);
      }
    }
  }
}

void Overworld::Minimap::DrawLayer(sf::RenderTarget& target, sf::Shader& shader, sf::RenderStates states, Overworld::Map& map, size_t index) {
  auto& layer = map.GetLayer(index);

  // TODO: render SOME objects that are overlaying the map
  // if (!layer.IsVisible()) return;

  int rows = (int)map.GetRows();
  int cols = (int)map.GetCols();
  auto tileSize = map.GetTileSize();

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      auto tile = layer.GetTile(j, i);
      if (tile->gid == 0) continue;

      auto tileMeta = map.GetTileMeta(tile->gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

      // hidden
      if (tileMeta->type == TileType::invisible) continue;

      if(index > 0 && map.IgnoreTileAbove((float)j, (float)i, (int)index - 1)) continue;

      auto& tileSprite = tileMeta->sprite;
      const auto subRect = tileSprite.getTextureRect();

      const auto originalOrigin = tileSprite.getOrigin();
      tileSprite.setOrigin(sf::Vector2f(sf::Vector2i(
        subRect.width / 2,
        tileSize.y / 2
      )));

      sf::Vector2i pos((j * tileSize.x) / 2, i * tileSize.y);
      auto ortho = map.WorldToScreen(sf::Vector2f(pos));
      auto tileOffset = sf::Vector2f(sf::Vector2i(
        -tileSize.x / 2 + subRect.width / 2,
        tileSize.y + tileSize.y / 2 - subRect.height
      ));

      tileSprite.setPosition(ortho + tileMeta->drawingOffset + tileOffset);
      tileSprite.setRotation(tile->rotated ? 90.0f : 0.0f);
      tileSprite.setScale(
        tile->flippedHorizontal ? -1.0f : 1.0f,
        tile->flippedVertical ? -1.0f : 1.0f
      );

      // pass info to shader
      auto center = sf::Vector2f(float(subRect.left), float(subRect.top));

      if(tile->flippedHorizontal) {
        center.x += float(subRect.width) - (tileSize.x / 2.0f);
        center.x += tileMeta->drawingOffset.x;
      } else {
        center.x += tileSize.x / 2.0f;
        center.x += -tileMeta->drawingOffset.x;
      }

      if (tile->flippedVertical) {
        center.y += tileSize.y / 2.0f;
        center.y += tileMeta->drawingOffset.y;
      } else {
        center.y += subRect.height - (tileSize.y / 2.0f);
        center.y += -tileMeta->drawingOffset.y;
      }

      sf::Vector2f sizeUv(tileSprite.getTexture()->getSize());
      shader.setUniform("center", sf::Glsl::Vec2(center.x / sizeUv.x, center.y / sizeUv.y));
      shader.setUniform("tileSize", sf::Glsl::Vec2((tileSize.x + 1) / sizeUv.x, (tileSize.y + 1) / sizeUv.y));
      shader.setUniform("mask", tileMeta->type != TileType::stairs);

      // draw
      target.draw(tileSprite, states);

      // reset origin
      tileSprite.setOrigin(originalOrigin);
    }
  }
}

Overworld::Minimap::Minimap()
{
  auto markerTexture = Textures().LoadFromFile("resources/ow/minimap/markers.png");
  auto markerAnimation = Animation("resources/ow/minimap/markers.animation");

  auto initMarker = [&] (SpriteProxyNode& node, const std::string& state) {
    node.setTexture(markerTexture);
    markerAnimation << state;
    markerAnimation.Refresh(node.getSprite());
  };

  initMarker(player, "actor");
  initMarker(home, "home");
  initMarker(warp, "warp");
  initMarker(board, "board");
  initMarker(shop, "shop");
  initMarker(conveyor, "conveyor");
  initMarker(arrow, "arrow");
  overlay.setTexture(Textures().LoadFromFile("resources/ow/minimap/mm_over.png"));
  overlayArrows.setTexture(Textures().LoadFromFile("resources/ow/minimap/mm_over_arrows.png"));

  // dark blueish
  bgColor = sf::Color(24, 56, 104, 255);
  rectangle = sf::RectangleShape({ 240,160 });
  rectangle.setFillColor(bgColor);
}

Overworld::Minimap::~Minimap()
{
}

void Overworld::Minimap::ResetPanning()
{
  panning = {};
}

void Overworld::Minimap::Pan(const sf::Vector2f& amount)
{
  panning += amount;

  float maxx = bakedMap.getLocalBounds().width * 0.5f;
  float maxy = bakedMap.getLocalBounds().height * 0.5f;
  float minx = -maxx;
  float miny = -maxy;

  if (panning.x < minx) {
    panning.x = minx;
  }

  if (panning.x > maxx) {
    panning.x = maxx;
  }

  if (panning.y < miny) {
    panning.y = miny;
  }

  if (panning.y > maxy) {
    panning.y = maxy;
  }
}

void Overworld::Minimap::SetPlayerPosition(const sf::Vector2f& pos, bool isConcealed)
{
  player.setColor(isConcealed ? PLAYER_COLOR * CONCEALED_COLOR : PLAYER_COLOR);

  if (largeMapControls) {
    auto newpos = panning + (-pos * this->scaling);
    player.setPosition((240.f * 0.5f)+panning.x, (160.f * 0.5f)+panning.y);
    this->bakedMap.setPosition(newpos + offset);
  }
  else {
    auto newpos = pos * this->scaling;
    player.setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
    this->bakedMap.setPosition(0,0);
  }
}

void Overworld::Minimap::AddPlayerMarker(std::shared_ptr<Overworld::Minimap::PlayerMarker> marker) {
  CopyFrame(*marker, player);
  marker->SetLayer(-1);
  bakedMap.AddNode(marker);
  playerMarkers.push_back(marker);
}

void Overworld::Minimap::UpdatePlayerMarker(Overworld::Minimap::PlayerMarker& playerMarker, sf::Vector2f pos, bool isConcealed)
{
  auto markerColor = playerMarker.GetMarkerColor();
  playerMarker.setColor(isConcealed ? markerColor * CONCEALED_COLOR : markerColor);
  pos *= this->scaling;
  playerMarker.setPosition(pos.x + (240.f * 0.5f) - offset.x, pos.y + (160.f * 0.5f) - offset.y);
}

void Overworld::Minimap::RemovePlayerMarker(std::shared_ptr<Overworld::Minimap::PlayerMarker> playerMarker) {
  auto it = std::find(playerMarkers.begin(), playerMarkers.end(), playerMarker);

  if (it != playerMarkers.end()) {
    playerMarkers.erase(it);
  }

  bakedMap.RemoveNode(playerMarker.get());
}

void Overworld::Minimap::ClearIcons()
{
  for(auto& marker : mapMarkers) {
    bakedMap.RemoveNode(marker.get());
  }

  mapMarkers.clear();
}

void Overworld::Minimap::AddHomepagePosition(const sf::Vector2f& pos, bool isConcealed)
{
  std::shared_ptr<SpriteProxyNode> newHome = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newHome, home);

  AddMarker(newHome, pos, isConcealed);
  mapMarkers.push_back(newHome);
}

void Overworld::Minimap::AddWarpPosition(const sf::Vector2f& pos, bool isConcealed)
{
  std::shared_ptr<SpriteProxyNode> newWarp = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newWarp, warp);

  AddMarker(newWarp, pos, isConcealed);
  mapMarkers.push_back(newWarp);
}

void Overworld::Minimap::AddBoardPosition(const sf::Vector2f& pos, bool flip, bool isConcealed)
{
  std::shared_ptr<SpriteProxyNode> newBoard = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newBoard, board);

  newBoard->setScale(flip ? -1.f : 1.f, 1.f);

  AddMarker(newBoard, pos, isConcealed);
  mapMarkers.push_back(newBoard);
}

void Overworld::Minimap::AddShopPosition(const sf::Vector2f& pos, bool isConcealed)
{
  std::shared_ptr<SpriteProxyNode> newShop = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newShop, shop);
  AddMarker(newShop, pos, isConcealed);
  mapMarkers.push_back(newShop);
}

void Overworld::Minimap::AddConveyorPosition(const sf::Vector2f& pos, Direction direction, bool isConcealed)
{
  std::shared_ptr<SpriteProxyNode> newConveyor = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newConveyor, conveyor);

  switch (direction)
  {
  case Direction::up_left:
    newConveyor->setScale(-1, -1);
    break;
  case Direction::up_right:
    newConveyor->setScale(1, -1);
    break;
  case Direction::down_left:
    newConveyor->setScale(-1, 1);
    break;
  }

  AddMarker(newConveyor, pos, isConcealed);
  mapMarkers.push_back(newConveyor);
}

void Overworld::Minimap::AddArrowPosition(const sf::Vector2f& pos, Direction direction)
{
  std::shared_ptr<SpriteProxyNode> newArrow = std::make_shared<SpriteProxyNode>();
  CopyFrame(*newArrow, arrow);

  switch (direction)
  {
  case Direction::up_left:
    newArrow->setScale(-1, -1);
    break;
  case Direction::up_right:
    newArrow->setScale(1, -1);
    break;
  case Direction::down_left:
    newArrow->setScale(-1, 1);
    break;
  }

  AddMarker(newArrow, pos, false);
  mapMarkers.push_back(newArrow);
}

void Overworld::Minimap::AddMarker(const std::shared_ptr<SpriteProxyNode>& marker, const sf::Vector2f& pos, bool isConcealed) {
  if (isConcealed) {
    marker->setColor(CONCEALED_COLOR);
  }

  auto newpos = pos * this->scaling;
  marker->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  marker->SetLayer(-1);
  bakedMap.AddNode(marker);
}

void Overworld::Minimap::Open() {
  ResetPanning();
  Menu::Open();
}

void Overworld::Minimap::HandleInput(InputManager& input, sf::Vector2f mousePos) {
  sf::Vector2f panningOffset;

  if (input.Has(InputEvents::held_ui_left)) {
    panningOffset.x -= 1.f;
  }

  if (input.Has(InputEvents::held_ui_right)) {
    panningOffset.x += 1.f;
  }

  if (input.Has(InputEvents::held_ui_up)) {
    panningOffset.y -= 1.f;
  }

  if (input.Has(InputEvents::held_ui_down)) {
    panningOffset.y += 1.f;
  }

  if (input.GetConfigSettings().GetInvertMinimap()) {
    panningOffset.x *= -1.f;
    panningOffset.y *= -1.f;
  }

  if (input.Has(InputEvents::pressed_map)) {
    Close();
  }

  Pan(panningOffset);
}

void Overworld::Minimap::draw(sf::RenderTarget& surface, sf::RenderStates states) const
{
  states.transform *= getTransform();
  surface.draw(rectangle, states);
  surface.draw(this->bakedMap, states);
  surface.draw(this->player, states);
  surface.draw(this->overlay, states);

  if (!largeMapControls) return;
  surface.draw(this->overlayArrows, states);
}

sf::Color Overworld::Minimap::PlayerMarker::GetMarkerColor() {
  return markerColor;
}

void Overworld::Minimap::PlayerMarker::SetMarkerColor(sf::Color color) {
  markerColor = color;
}
