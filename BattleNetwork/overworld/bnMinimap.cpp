#include "bnMinimap.h"
#include <Swoosh/EmbedGLSL.h>
#include <cmath>
#include "../bnTextureResourceManager.h"

static void CopyTextureAndOrigin(SpriteProxyNode& dest, const SpriteProxyNode& source) {
  dest.setTexture(source.getTexture());
  dest.setOrigin(source.getOrigin());
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

Overworld::Minimap Overworld::Minimap::CreateFrom(const std::string& name, Map& map)
{
  Minimap minimap;
  minimap.name = name;

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

  minimap.scaling = tileHeight / tileSize.y;

  // how many times could this map fit into the screen? make that many.
  sf::Vector2f texsizef = {
    std::ceil(mapScreenUnitDimensions.x * tileSize.x * minimap.scaling * 0.5f),
    std::ceil(mapScreenUnitDimensions.y * tileSize.y * minimap.scaling * 0.5f) + 1.0f
  };

  auto textureSize = sf::Vector2i(texsizef);

  textureSize.x = std::max(screenSize.x, textureSize.x);
  textureSize.y = std::max(screenSize.y, textureSize.y);

  // texture does not fit on screen, allow large map controls
  minimap.largeMapControls = textureSize.x > screenSize.x || textureSize.y > screenSize.y;

  if (!texture.create(textureSize.x, textureSize.y)) return minimap;

  // fill with background color
  texture.clear(sf::Color(0,0,0,0));

  // shader pass transforms opaque pixels into purple hues
  std::string recolor = GLSL(
    110,
    uniform sampler2D texture;
    uniform vec2 center;
    uniform vec2 tileSize;
    uniform vec4 finalColor;
    uniform int mask;

    void main() {

      vec2 pos = gl_TexCoord[0].xy; // pos is the uv coord (subrect)
      vec4 incolor = texture2D(texture, pos).rgba;
      vec4 outcolor;

      pos.x = center.x - pos.x;
      pos.y = center.y - pos.y;

      if (mask == 0) {
        outcolor = vec4(finalColor.r*0.60, finalColor.g*1.20, finalColor.b*0.60, ceil(incolor.a));
      }
      else {
        outcolor = finalColor * ceil(incolor.a);
      }

      if (mask > 0 && abs(2.0 * pos.x / tileSize.x) + abs(2.0 * pos.y / tileSize.y) > 1.0)
       discard;

      gl_FragColor = outcolor;
    }
  );

  std::string edgeDetection = GLSL(
    110,
    uniform sampler2D texture;
    uniform float resolutionW;
    uniform float resolutionH;

    void main(void)
    {
      float dx = 1.0 / resolutionW;
      float dy = 1.0 / resolutionH;

      vec2 pos = gl_TexCoord[0].xy;
      vec4 incolor = texture2D(texture, pos).rgba;

      // this just checks for alpha
      vec4 top = texture2D(texture, vec2(pos.x, pos.y - dy));
      vec4 left = texture2D(texture, vec2(pos.x - dx, pos.y));
      vec4 right = texture2D(texture, vec2(pos.x + dx, pos.y));
      vec4 down = texture2D(texture, vec2(pos.x, pos.y + dy));

      vec4 n = max(down, max(top, max(left, right)));
      float m = n.a*(1.0 - incolor.a);

      gl_FragColor = m * (n * 0.90);
    }
  );

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

  sf::Shader shader;
  shader.loadFromMemory(recolor, sf::Shader::Type::Fragment);
  states.shader = &shader;

  // guestimate best fit "center" of the map
  auto layerDimensions = map.TileToWorld({ (float)map.GetCols(), (float)map.GetRows() });
  sf::Vector3f mapDimensions = {
    layerDimensions.x, layerDimensions.y,
    (float)map.GetLayerCount() - 1.0f
  };

  // move the map to the center of the screen and fit
  // TODO: chunk the minimap for large map
  sf::Vector2f center = map.WorldToScreen(sf::Vector3f(mapDimensions. x, mapDimensions.y, mapDimensions.z * 2.0f) * 0.5f) * minimap.scaling;
  minimap.offset = center;

  const int maxLayerCount = (int)map.GetLayerCount();

  // apply transforms
  states.transform.translate((240.f * 0.5f) - center.x, (160.f * 0.5f) - center.y);
  states.transform.scale(minimap.scaling, minimap.scaling);

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

    shader.setUniform("finalColor", lerpColor(i, maxLayerCount, layer1Color, layerMaxColor));

    // draw layer (Applies masking)
    minimap.DrawLayer(texture, shader, states, map, i);

    // iso layers are offset (prepare for the next layer)
    states.transform.translate(0.f, -tileSize.y * 0.5f);
  }

  // Grab image data to make a texture
    // NOTE: gpu <-> cpu is a costly process. do this sparringly.
  texture.display();
  sf::Texture tempTex = texture.getTexture();
  sf::Sprite temp(tempTex);

  // do a second pass for edge detection
  states.transform = sf::Transform::Identity;
  shader.loadFromMemory(edgeDetection, sf::Shader::Type::Fragment);
  shader.setUniform("resolutionW", (float)textureSize.x);
  shader.setUniform("resolutionH", (float)textureSize.y);
  texture.draw(temp, states);

  texture.display();

  // set the final texture
  minimap.bakedMap.setTexture(std::make_shared<sf::Texture>(texture.getTexture()));

  minimap.FindTileMarkers(map);

  return minimap;
}

void Overworld::Minimap::FindTileMarkers(Map& map) {
  auto layerCount = map.GetLayerCount();
  auto cols = map.GetCols();
  auto rows = map.GetRows();

  for (auto i = 0; i < layerCount; i++) {
    auto& layer = map.GetLayer(i);

    for (auto col = 0; col < cols; col++) {
      for (auto row = 0; row < rows; row++) {
        auto& tile = layer.GetTile(col, row);
        auto tileMeta = map.GetTileMeta(tile.gid);

        if (!tileMeta) {
          continue;
        }

        if (tileMeta->type == "Conveyor") {
          auto pos = sf::Vector2f(col, row);
          pos.x += 0.5f;
          pos.y += 0.5f;

          auto worldPos = map.TileToWorld(pos);
          auto direction = FromString(tileMeta->customProperties.GetProperty("Direction"));

          if (tile.flippedHorizontal) {
            direction = FlipHorizontal(direction);
          }

          if (tile.flippedVertical) {
            direction = FlipVertical(direction);
          }

          auto screenPos = map.WorldToScreen({ worldPos.x, worldPos.y, float(i) });

          AddConveyorPosition(screenPos, direction);
        }
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
      auto& tile = layer.GetTile(j, i);
      if (tile.gid == 0) continue;

      auto tileMeta = map.GetTileMeta(tile.gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

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
      tileSprite.setRotation(tile.rotated ? 90.0f : 0.0f);
      tileSprite.setScale(
        tile.flippedHorizontal ? -1.0f : 1.0f,
        tile.flippedVertical ? -1.0f : 1.0f
      );

      // pass info to shader
      auto center = sf::Vector2f(float(subRect.left), float(subRect.top));

      if(tile.flippedHorizontal) {
        center.x += float(subRect.width) - (tileSize.x / 2.0f);
        center.x += tileMeta->drawingOffset.x;
      } else {
        center.x += tileSize.x / 2.0f;
        center.x += -tileMeta->drawingOffset.x;
      }

      if (tile.flippedVertical) {
        center.y += tileSize.y / 2.0f;
        center.y += tileMeta->drawingOffset.y;
      } else {
        center.y += subRect.height - (tileSize.y / 2.0f);
        center.y += -tileMeta->drawingOffset.y;
      }

      sf::Vector2f sizeUv(tileSprite.getTexture()->getSize());
      shader.setUniform("center", sf::Glsl::Vec2(center.x / sizeUv.x, center.y / sizeUv.y));
      shader.setUniform("tileSize", sf::Glsl::Vec2((tileSize.x + 1) / sizeUv.x, (tileSize.y + 1) / sizeUv.y));
      shader.setUniform("mask", tileMeta->type != "Stairs");

      // draw
      target.draw(tileSprite, states);

      // reset origin
      tileSprite.setOrigin(originalOrigin);
    }
  }
}

void Overworld::Minimap::EnforceTextureSizeLimits()
{
  player.setTextureRect({ 0, 0, 6, 8 });
  hp.setTextureRect({ 0, 0, 8, 8 });
  warp.setTextureRect({ 0, 0, 8, 6 });
  board.setTextureRect({ 0, 0, 6, 7 });
  shop.setTextureRect({ 0, 0, 6, 7 });
  conveyor.setTextureRect({ 0, 0, 6, 3 });
  overlay.setTextureRect({ 0, 0, 240, 160 });

  player.setOrigin({ 3, 8 });
  hp.setOrigin({ 4, 4 });
  warp.setOrigin({ 4, 3 });
  board.setOrigin({ 3, 7 });
  shop.setOrigin({ 3, 7 });
  conveyor.setOrigin({ 3, 1.5 });
}

Overworld::Minimap::Minimap()
{
  player.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_pos.png"));
  hp.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_hp.png"));
  overlay.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_over.png"));
  arrows.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_over_arrows.png"));
  warp.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_warp.png"));
  board.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_board.png"));
  shop.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_shop.png"));
  conveyor.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_conveyor.png"));
  EnforceTextureSizeLimits();

  // dark blueish
  bgColor = sf::Color(24, 56, 104, 255);
  rectangle = sf::RectangleShape({ 240,160 });
  rectangle.setFillColor(bgColor);
}

Overworld::Minimap::Minimap(const Minimap& rhs)
{
  this->operator=(rhs);
}

Overworld::Minimap::~Minimap()
{
}

Overworld::Minimap& Overworld::Minimap::operator=(const Minimap& rhs)
{
  player.setTexture(rhs.player.getTexture(), false);
  hp.setTexture(rhs.hp.getTexture(), false);
  bakedMap.setTexture(rhs.bakedMap.getTexture(), false);
  arrows.setTexture(rhs.arrows.getTexture(), false);
  warp.setTexture(rhs.warp.getTexture(), false);
  player.setPosition(rhs.player.getPosition());
  hp.setPosition(rhs.hp.getPosition());
  bakedMap.setPosition(rhs.bakedMap.getPosition());
  EnforceTextureSizeLimits();

  bgColor = rhs.bgColor;
  scaling = rhs.scaling;
  offset = rhs.offset;
  name = rhs.name;
  rectangle = rhs.rectangle;
  largeMapControls = rhs.largeMapControls;

  std::vector<SceneNode*> oldNodes = bakedMap.GetChildNodes();
  for (auto old : oldNodes) {
    bakedMap.RemoveNode(old);
  }

  auto& rhsNodes = rhs.bakedMap.GetChildNodes();
  auto hpIter = std::find(rhsNodes.begin(), rhsNodes.end(), &rhs.hp);

  if (hpIter != rhsNodes.end()) {
    bakedMap.AddNode(&hp);
  }

  for (auto& rhsMarker : rhs.markers) {
    auto marker = std::make_shared<SpriteProxyNode>();
    CopyTextureAndOrigin(*marker, *rhsMarker);
    marker->setScale(rhsMarker->getScale());
    marker->setPosition(rhsMarker->getPosition());
    marker->SetLayer(-1);
    bakedMap.AddNode(marker.get());
    markers.push_back(marker);
  }

  return *this;
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

void Overworld::Minimap::SetPlayerPosition(const sf::Vector2f& pos)
{
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

void Overworld::Minimap::SetHomepagePosition(const sf::Vector2f& pos)
{
  auto newpos = pos * this->scaling;
  hp.setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  bakedMap.RemoveNode(&hp);
  bakedMap.AddNode(&hp); // follow the map
  hp.SetLayer(-1); // on top of the map
}

void Overworld::Minimap::ClearIcons()
{
  bakedMap.RemoveNode(&hp);

  for(auto& marker : markers) {
    bakedMap.RemoveNode(marker.get());
  }

  markers.clear();
}

void Overworld::Minimap::AddWarpPosition(const sf::Vector2f& pos)
{
  std::shared_ptr<SpriteProxyNode> newWarp = std::make_shared<SpriteProxyNode>();
  CopyTextureAndOrigin(*newWarp, warp);

  AddMarker(newWarp, pos);
}

void Overworld::Minimap::AddBoardPosition(const sf::Vector2f& pos, bool flip)
{
  std::shared_ptr<SpriteProxyNode> newBoard = std::make_shared<SpriteProxyNode>();
  CopyTextureAndOrigin(*newBoard, board);

  newBoard->setScale(flip ? -1.f : 1.f, 1.f);

  AddMarker(newBoard, pos);
}

void Overworld::Minimap::AddShopPosition(const sf::Vector2f& pos)
{
  std::shared_ptr<SpriteProxyNode> newShop = std::make_shared<SpriteProxyNode>();
  CopyTextureAndOrigin(*newShop, shop);
  AddMarker(newShop, pos);
}

void Overworld::Minimap::AddConveyorPosition(const sf::Vector2f& pos, Direction direction)
{
  std::shared_ptr<SpriteProxyNode> newConveyor = std::make_shared<SpriteProxyNode>();
  CopyTextureAndOrigin(*newConveyor, conveyor);

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

  AddMarker(newConveyor, pos);
}

void Overworld::Minimap::AddMarker(const std::shared_ptr<SpriteProxyNode>& marker, const sf::Vector2f& pos) {
  auto newpos = pos * this->scaling;
  marker->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  marker->SetLayer(-1);
  markers.push_back(marker);
  bakedMap.AddNode(markers.back().get());
}

void Overworld::Minimap::draw(sf::RenderTarget& surface, sf::RenderStates states) const
{
  states.transform *= getTransform();
  surface.draw(rectangle, states);
  surface.draw(this->bakedMap, states);
  surface.draw(this->player, states);
  surface.draw(this->overlay, states);

  if (!largeMapControls) return;
  surface.draw(this->arrows, states);
}
