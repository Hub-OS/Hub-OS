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
  const auto gridSize = sf::Vector2i(maxTileHeight * map.GetCols(), maxTileHeight * map.GetRows());

  // how many times could this map fit into the screen? make that many.
  auto textureSize = sf::Vector2i( screenSize.x * (static_cast<float>(gridSize.x) / screenSize.x), 
    screenSize.y * (static_cast<float>(gridSize.y) / screenSize.y));

  textureSize.x = std::max(screenSize.x, textureSize.x);
  textureSize.y = std::max(screenSize.y, textureSize.y);

  if (!texture.create(textureSize.x, textureSize.y)) return minimap;

  // fill with background color
  texture.clear(sf::Color(0,0,0,0));

  // shader pass transforms opaque pixels into purple hues
  std::string recolor = GLSL(
    110,
    uniform sampler2D texture;
    uniform vec2 subrect;
    uniform vec2 tileSize;

    void main() {
      vec2 pos = gl_TexCoord[0].xy; // pos is the uv coord (subrect)
      vec4 incolor = texture2D(texture, pos).rgba;

      // these are uv coordinate checks
      // if (abs(pos.x) + abs(2.0 * pos.y) > 1.0)
      //   discard;

      //float x = pos.x  tileSize.x;
      //float y = pos.y % tileSize.y;
      //if (abs(tileSize.x-pos.x) + abs(pos.y) > 1.0)
      //  discard;

      // 152, 144, 224
      vec4 outcolor = vec4(0.596, 0.564, 0.878, 1.0) * incolor.a;
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

      // 120, 112, 192
      vec4 edgeColor = vec4(0.470, 0.439, 0.752, 1.0);
      vec2 pos = gl_TexCoord[0].xy;
      vec4 incolor = texture2D(texture, pos).rgba;

      float top = 1.0-texture2D(texture, vec2(pos.x, pos.y-dy)).a;
      float left = 1.0-texture2D(texture, vec2(pos.x-dx, pos.y)).a;
      float right = 1.0-texture2D(texture, vec2(pos.x+dx, pos.y)).a;
      float down = 1.0-texture2D(texture, vec2(pos.x, pos.y+dy)).a;
      float n = max(top, max(left, max(right, down))) *(incolor.a);

      gl_FragColor = (edgeColor*n) + ((1.0-n)*incolor);
    }
  );

  sf::Shader shader;
  shader.loadFromMemory(recolor, sf::Shader::Type::Fragment);
  states.shader = &shader;
  //auto spriteSheetSz = map.GetTileset("floor")->texture->getSize();

  // guestimate best fit "center" of the map
  auto tileSize = map.GetTileSize();

  //sf::Vector2f tileUV = sf::Vector2f(static_cast<float>(tileSize.x) / spriteSheetSz.x, static_cast<float>(tileSize.y) / spriteSheetSz.y);
  //shader.setUniform("tileSize", tileUV);

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

  // apply transforms
  map.setScale(minimap.scaling, minimap.scaling);
  map.setPosition((240.f*0.5f)-minimap.offset.x, (160.f*0.5f)-minimap.offset.y);

  states.transform *= map.getTransform();

  // draw. every layer passes through the shader
  for (auto i = 0; i < map.GetLayerCount(); i++) {
    minimap.DrawLayer(texture, states, map, i);

    // translate next layer
    states.transform.translate(0.f, -tileSize.y * 0.5f);
  }

  // revert original settings
  map.setScale(oldscale);
  map.setPosition(oldposition);
  map.setOrigin(oldorigin);

  // Grab image data to make a texture
  // NOTE: gpu <-> cpu is a costly process. do this sparringly.
  texture.display();
  sf::Texture tempTex = texture.getTexture();
  sf::Sprite temp(tempTex);

  // do a second pass for edge detection
  shader.loadFromMemory(edgeDetection, sf::Shader::Type::Fragment);
  states.transform = sf::Transform::Identity;
  shader.setUniform("resolutionW", (float)textureSize.x);
  shader.setUniform("resolutionH", (float)textureSize.y);

  texture.clear(sf::Color(0,0,0,0)); // clear transparent pixels
  texture.draw(temp, states);
  texture.display();

  // set the final texture
  minimap.bakedMap.setTexture(std::make_shared<sf::Texture>(texture.getTexture()));
  return minimap;
}

void Overworld::Minimap::DrawLayer(sf::RenderTarget& target, sf::RenderStates states, Overworld::Map& map, size_t index) {
  auto& layer = map.GetLayer(index);

  // TODO: render SOME objects that are overlaying the map
  // if (!layer.IsVisible()) return;

  auto rows = map.GetRows();
  auto cols = map.GetCols();
  auto tileSize = map.GetTileSize();

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      auto& tile = layer.GetTile(j, i);
      if (tile.gid == 0) continue;

      auto tileMeta = map.GetTileMeta(tile.gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

      auto& tileSprite = tileMeta->sprite;
      auto spriteBounds = tileSprite.getLocalBounds();

      auto subrect = tileSprite.getTextureRect();
      sf::Vector2f subrectUV = sf::Vector2f((float)subrect.left / tileSize.x, (float)subrect.top / tileSize.y);
      states.shader->setUniform("subrect", subrectUV);

      auto originalOrigin = tileSprite.getOrigin();
      tileSprite.setOrigin(sf::Vector2f(sf::Vector2i(
        (int)spriteBounds.width / 2,
        tileSize.y / 2
      )));

      sf::Vector2i pos((j * tileSize.x) / 2, i * tileSize.y);
      auto ortho = map.WorldToScreen(sf::Vector2f(pos));
      auto tileOffset = sf::Vector2f(sf::Vector2i(
        -tileSize.x / 2 + (int)spriteBounds.width / 2,
        tileSize.y + tileSize.y / 2 - (int)spriteBounds.height
      ));

      tileSprite.setPosition(ortho + tileMeta->drawingOffset + tileOffset);
      tileSprite.setRotation(tile.rotated ? 90.0f : 0.0f);
      tileSprite.setScale(
        tile.flippedHorizontal ? -1.0f : 1.0f,
        tile.flippedVertical ? -1.0f : 1.0f
      );

      target.draw(tileSprite, states);

      tileSprite.setOrigin(originalOrigin);
    }
  }
}

void Overworld::Minimap::EnforceTextureSizeLimits()
{
  player.setTextureRect({ 0, 0, 6, 8 });
  hp.setTextureRect({ 0, 0, 8, 8 });
  warp.setTextureRect({ 0, 0, 8, 6 });
  overlay.setTextureRect({ 0, 0, 240, 160 });

  player.setOrigin({ 3, 6 });
  hp.setOrigin({ 4, 4 });
  warp.setOrigin({ 4, 3 });
}

Overworld::Minimap::Minimap()
{
  player.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_pos.png"));
  hp.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_hp.png"));
  overlay.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_over.png"));
  arrows.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_over_arrows.png"));
  warp.setTexture(Textures().LoadTextureFromFile("resources/ow/minimap/mm_warp.png"));
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
  rectangle = rhs.rectangle;
  largeMapControls = rhs.largeMapControls;
  offset = rhs.offset;
  name = rhs.name;

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
  bakedMap.AddNode(&hp); // follow the map
  hp.SetLayer(-1); // on top of the map
}

void Overworld::Minimap::AddWarpPosition(const sf::Vector2f& pos)
{
  auto newpos = pos * this->scaling;
  std::shared_ptr<SpriteProxyNode> newWarp = std::make_shared<SpriteProxyNode>();
  newWarp->setTexture(warp.getTexture());
  newWarp->setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
  newWarp->SetLayer(-1);
  warps.push_back(newWarp);
  bakedMap.AddNode(warps.back().get());
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
