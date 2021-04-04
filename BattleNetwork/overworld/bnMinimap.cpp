#include "bnMinimap.h"
#include <Swoosh/EmbedGLSL.h>
#include "../bnTextureResourceManager.h"

Overworld::Minimap Overworld::Minimap::CreateFrom(const std::string& name, Map& map)
{
  Minimap minimap;
  minimap.name = name;

  // save transforms to revert after drawing
  sf::Vector2f oldscale = map.getScale();
  sf::Vector2f oldposition = map.getPosition();
  sf::Vector2f oldorigin = map.getOrigin();

  // prepare a render texture to write to
  sf::RenderTexture texture;
  sf::RenderStates states;

  const float maxTileHeight = 10.f;
  const float minTileHeight = 4.f;
  const auto screenSize = sf::Vector2i{ 240, 160 };
  const auto gridSize = sf::Vector2i(int(maxTileHeight * map.GetCols()), int(maxTileHeight * map.GetRows()));

  // how many times could this map fit into the screen? make that many.
  sf::Vector2f texsizef = { screenSize.x * (gridSize.x / float(screenSize.x)), screenSize.y * (gridSize.y / float(screenSize.y)) };
  auto textureSize = sf::Vector2i((int)texsizef.x, (int)texsizef.y);

  textureSize.x = std::max(screenSize.x, textureSize.x);
  textureSize.y = std::max(screenSize.y, textureSize.y);

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
        outcolor = vec4(finalColor.r*0.60, finalColor.g*1.20, finalColor.b*0.60, incolor.a);
      }
      else {
        outcolor = finalColor * incolor.a;
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
    uniform float delta;
    uniform vec4 edgeColor;

    void main(void)
    {
      float dx = 1.0 / resolutionW;
      float dy = 1.0 / resolutionH;

      vec2 pos = gl_TexCoord[0].xy;
      vec4 incolor = texture2D(texture, pos).rgba;

      /*
      * NOTE: checks difference in color for game-accurate map generation.
      */
      
      /*float g = incolor.g;
      float top = texture2D(texture, vec2(pos.x, pos.y-dy)).g;
      float left = texture2D(texture, vec2(pos.x-dx, pos.y)).g;
      float right = texture2D(texture, vec2(pos.x+dx, pos.y)).g;
      float down = texture2D(texture, vec2(pos.x, pos.y+dy)).g;
    
      float maxDiff = g - min(down, min(top, min(left, right)));

      /*if (maxDiff >= delta-0.008) {
        gl_FragColor = (incolor * 0.75).rgba;
      }
      else {
        gl_FragColor = incolor;
      }*/

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
  auto tileSize = map.GetTileSize();

  float tilex = tileSize.x * map.GetCols() * 0.5f;
  float tiley = tileSize.y * map.GetRows() * 1.0f;
  sf::Vector2f center = map.WorldToScreen({ tilex, tiley }) * 0.5f;

  // move the map to the center of the screen and fit
  // TODO: chunk the minimap for large map
  float mapHeight = (map.GetRows() + map.GetCols()) * (tileSize.y * 0.5f);
  auto targetTileHeight = maxTileHeight *std::min(1.0f, 160.f / (mapHeight * 0.30f));

  if (targetTileHeight <= minTileHeight) {
    targetTileHeight = minTileHeight;
    minimap.largeMapControls = true;
  }

  minimap.scaling = (targetTileHeight / tileSize.y);
  minimap.offset = center * minimap.scaling;
  minimap.offset.y = minimap.offset.y + (map.GetLayerCount() * minimap.scaling);

  // apply transforms
  map.setScale(minimap.scaling, minimap.scaling);
  map.setPosition((240.f*0.5f)-minimap.offset.x, (160.f*0.5f)-minimap.offset.y);

  states.transform *= map.getTransform();

  const int maxLayerCount = (int)map.GetLayerCount();

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
  shader.setUniform("edgeColor", sf::Glsl::Vec4(edge1Color));
  shader.setUniform("delta", (1.0f / (float)maxLayerCount)*((float)layer1Color.g/255.f));
  texture.draw(temp, states);

  texture.display();

  // set the final texture
  minimap.bakedMap.setTexture(std::make_shared<sf::Texture>(texture.getTexture()));

  // revert original settings
  map.setScale(oldscale);
  map.setPosition(oldposition);
  map.setOrigin(oldorigin);

  return minimap;
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

      if (tileMeta->type == "Stairs") {
        states.shader->setUniform("mask", false);
      }
      else {
        states.shader->setUniform("mask", true);
      }

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
  overlay.setTextureRect({ 0, 0, 240, 160 });

  player.setOrigin({ 3, 8 });
  hp.setOrigin({ 4, 4 });
  warp.setOrigin({ 4, 3 });
  board.setOrigin({ 3, 7 });
  shop.setOrigin({ 3, 7 });
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
  markers = rhs.markers;

  const auto oldNodes = bakedMap.GetChildNodes();
  for (auto old : oldNodes) {
    bakedMap.RemoveNode(old);
  }

  for (auto node : rhs.bakedMap.GetChildNodes()) {
    bakedMap.AddNode(node);
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
  auto newpos = pos * this->scaling;
  std::shared_ptr<SpriteProxyNode> newWarp = std::make_shared<SpriteProxyNode>();
  newWarp->setTexture(warp.getTexture());

  // getOrigin is returning { 0, 0 } ????
  // Logger::Logf("added: %f %f", warp.getOrigin().x, warp.getOrigin().y);
  newWarp->setOrigin({ 4, 3 });

  newWarp->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  newWarp->SetLayer(-1);
  markers.push_back(newWarp);
  bakedMap.AddNode(markers.back().get());
}

void Overworld::Minimap::AddBoardPosition(const sf::Vector2f& pos)
{
  auto newpos = pos * this->scaling;
  std::shared_ptr<SpriteProxyNode> newBoard = std::make_shared<SpriteProxyNode>();
  newBoard->setTexture(board.getTexture());
  newBoard->setOrigin({ 3, 7 });
  newBoard->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  newBoard->SetLayer(-1);
  markers.push_back(newBoard);
  bakedMap.AddNode(markers.back().get());
}

void Overworld::Minimap::AddShopPosition(const sf::Vector2f& pos)
{
  auto newpos = pos * this->scaling;
  std::shared_ptr<SpriteProxyNode> newShop = std::make_shared<SpriteProxyNode>();
  newShop->setTexture(shop.getTexture());
  newShop->setOrigin({ 3, 7 });
  newShop->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  newShop->SetLayer(-1);
  markers.push_back(newShop);
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
