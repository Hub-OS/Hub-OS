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

  if (!texture.create(240, 160)) return minimap;

  // fill with background color
  texture.clear(minimap.bgColor);

  // shader pass transforms opaque pixels into purple hues
  std::string program = GLSL(
    110,
    uniform sampler2D texture;

    void main() {
      vec4 outcol = texture2D(texture, gl_TexCoord[0].xy);

      // 152, 144, 224
      outcol = vec4(0.596, 0.564, 0.878, 1.0) * outcol.a;
      gl_FragColor = outcol;
    }
  );

  sf::Shader shader;
  shader.loadFromMemory(program, sf::Shader::Type::Fragment);
  states.shader = &shader;

  // guestimate best fit "center" of the map
  auto tileSize = map.GetTileSize();
  float tilex = tileSize.x * map.GetCols() * 0.5;
  float tiley = tileSize.y * map.GetRows() * 1.0f;
  sf::Vector2f center = map.WorldToScreen({ tilex, tiley }) * 0.5f;

  // move the map to the center of the screen and fit
  // TODO: chunk the minimap for large map
  const float maxTileHeight = 10.f;
  float mapHeight = (map.GetRows() + map.GetCols()) * (tileSize.y * 0.5f);
  auto targetTileHeight = maxTileHeight * std::min(1.0f, 160.f/(mapHeight*0.5f));
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
  minimap.bakedMapTex = std::make_shared<sf::Texture>(texture.getTexture());
  minimap.bakedMap.setTexture(*minimap.bakedMapTex, false);

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

      /*auto color = tileSprite.getColor();

      auto& [y, x] = PixelToRowCol(sf::Mouse::getPosition(*ENGINE.GetWindow()));

      bool hover = (y == i && x == j);

      if (hover) {
        tileSprite.setColor({ color.r, color.b, color.g, 200 });
      }*/

      if (/*cam && cam->IsInView(tileSprite)*/ true) {
        target.draw(tileSprite, states);
      }

      tileSprite.setOrigin(originalOrigin);
    }
  }
}

Overworld::Minimap::Minimap()
{
  playerTex = Textures().LoadTextureFromFile("resources/ow/minimap/mm_pos.png");
  hpTex = Textures().LoadTextureFromFile("resources/ow/minimap/mm_hp.png");
  overlayTex = Textures().LoadTextureFromFile("resources/ow/minimap/mm_over.png");

  player.setTextureRect({ 0, 0, 6, 8 });
  hp.setTextureRect({0, 0, 8, 8});
  overlay.setTextureRect({0, 0, 240, 160});

  player.setOrigin({ 3, 5 });
  hp.setOrigin({ 4, 4 });

  player.setTexture(*playerTex, false);
  hp.setTexture(*hpTex, false);
  overlay.setTexture(*overlayTex, false);

  // dark blueish
  bgColor = sf::Color(24, 56, 104, 255);
}

Overworld::Minimap::~Minimap()
{
}

void Overworld::Minimap::SetPlayerPosition(const sf::Vector2f& pos)
{
  auto newpos = pos * this->scaling;
  player.setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
}

void Overworld::Minimap::SetHomepagePosition(const sf::Vector2f& pos)
{
  this->validHpIcon = true;
  auto newpos = pos * this->scaling;
  hp.setPosition(newpos.x + (240.f * 0.5f) - offset.x, newpos.y + (160.f * 0.5f) - offset.y);
}

void Overworld::Minimap::draw(sf::RenderTarget& surface, sf::RenderStates states) const
{
  states.transform *= getTransform();
  surface.draw(this->bakedMap, states);

  if (this->validHpIcon) {
    surface.draw(this->hp, states);
  }

  surface.draw(this->player, states);

  surface.draw(this->overlay, states);
}
