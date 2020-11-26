#include <cmath>

#include "bnOverworldMap.h"

#include "../bnEngine.h"

namespace Overworld {
  Map::Map() :
    sf::Drawable(), sf::Transformable() {
    // NOTE: hard coded tilesets for now
    tileWidth = 62 + 2.5f; // calculated by hand. the offsets seem important.
    tileHeight = 32 + 0.5f; // calculated by hand. the offsets seem important.
  }

  void Map::Load(Map::Tileset tileset, Map::Tile** tiles, unsigned cols, unsigned rows)
  {
    this->tileset = tileset;
    this->tiles = tiles;
    this->cols = cols;
    this->rows = rows;
  }

  void Map::ToggleLighting(bool state) {
    enableLighting = state;

    if (!enableLighting) {
      for (int i = 0; i < lights.size(); i++) {
        delete lights[i];
      }

      lights.clear();
    }
  }

  const sf::Vector2f Map::ScreenToWorld(sf::Vector2f screen) const
  {
    return OrthoToIsometric(screen);
  }

  Map::~Map() {
    for (int i = 0; i < lights.size(); i++) {
      delete lights[i];
    }

    lights.clear();

    sprites.clear();
  }

  void Map::SetCamera(Camera* _camera) {
    cam = _camera;
  }

  void Map::AddLight(Overworld::Light * _light)
  {
    lights.push_back(_light);
  }

  void Map::AddSprite(SpriteProxyNode * _sprite, int layer)
  {
    sprites.push_back({ _sprite, layer });
  }

  void Map::RemoveSprite(const SpriteProxyNode * _sprite) {
    auto pos = std::find_if(sprites.begin(), sprites.end(), [_sprite](MapSprite in) { return in.node == _sprite; });

    if(pos != sprites.end())
      sprites.erase(pos);
  }

  void Map::Update(double elapsed)
  {
    std::sort(sprites.begin(), sprites.end(), 
        [](MapSprite A, MapSprite B)
      { 
        int A_layer_inv = -A.layer;
        int B_layer_inv = -B.layer;

        auto A_pos = A.node->getPosition();
        auto B_pos = B.node->getPosition();
        auto A_compare = A_pos.x + A_pos.y;
        auto B_compare = B_pos.x + B_pos.y;

        return std::tie(A_layer_inv, A_compare) < std::tie(B_layer_inv, B_compare);
      }
    );

  }

  void Map::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= this->getTransform();

    DrawTiles(target, states);
    DrawSprites(target, states);
  }

  const Map::Tile Map::GetTileAt(const sf::Vector2f& pos) const
  {
    unsigned x = pos.x / (tileWidth * 0.5f);
    unsigned y = pos.y / tileHeight;

    if (x >= 0 && x < cols && y >= 0 && y < rows) {
      return tiles[y][x];
    }

    return Map::Tile{};
  }

  void Map::SetTileAt(const sf::Vector2f& pos, const Tile& newTile)
  {
    unsigned x = pos.x / (tileWidth * 0.5f);
    unsigned y = pos.y / tileHeight;

    if (x >= 0 && x < cols && y >= 0 && y < rows) {
      tiles[y][x] = newTile;
    }
  }

  const std::vector<sf::Vector2f> Map::FindToken(const std::string& token)
  {
    std::vector<sf::Vector2f> results;

    for (unsigned i = 0; i < rows; i++) {
      for (unsigned j = 0; j < cols; j++) {
        if (tiles[i][j].token == token) {
          // find tile pos + half of width and height so the point is center tile
          results.push_back(sf::Vector2f((j * tileWidth * 0.5f)+(tileWidth*0.25f), (i * tileHeight)+(tileHeight*0.5f)));
        }
      }
    }

    return results;
  }

  void Map::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const {
    for (unsigned i = 0; i < rows; i++) {
      for (unsigned j = 0; j < cols; j++) {
        size_t ID = tiles[i][j].ID;

        if (ID == 0) continue; // reserved for empty tile

        sf::Sprite tileSprite = tileset.Graphic(ID);
        sf::Vector2f pos(j * tileWidth * 0.5f, i * tileHeight);
        auto iso = OrthoToIsometric(pos);
        iso = sf::Vector2f(iso.x, iso.y);
        tileSprite.setPosition(iso);

        auto color = tileSprite.getColor();
        
        auto& [y, x] = PixelToRowCol(sf::Mouse::getPosition(*ENGINE.GetWindow()));

        bool hover = (y == i && x == j);

        if (hover) {
          tileSprite.setColor({ color.r, color.b, color.g, 200 });
        }

        if (/*cam && cam->IsInView(tileSprite)*/ true) {
          target.draw(tileSprite, states);
        }

        tileSprite.setColor(color);
      }
    }
  }

  void Map::DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const {
    for (int i = 0; i < sprites.size(); i++) {
      auto ortho = sprites[i].node->getPosition();
      auto iso = OrthoToIsometric(ortho);
      iso = sf::Vector2f(iso.x, iso.y);

      auto tileSprite = sprites[i].node;
      tileSprite->setPosition(iso);

      if (/*cam && cam->IsInView(tileSprite->getSprite())*/ true) {
        target.draw(*tileSprite, states);
      }

      tileSprite->setPosition(ortho);
    }
  }
  const sf::Vector2f Map::OrthoToIsometric(const sf::Vector2f& ortho) const {
    sf::Vector2f iso{};
    iso.x = (ortho.x - ortho.y);
    iso.y = (ortho.x + ortho.y) * 0.5f;

    return iso;
  }

  const sf::Vector2f Map::IsoToOrthogonal(const sf::Vector2f& iso) const {
    sf::Vector2f ortho{};
    ortho.x = (2.0f * iso.y + iso.x) * 0.5f;
    ortho.y = (2.0f * iso.y - iso.x) * 0.5f;

    return ortho;
  }

  const sf::Vector2i Map::GetTileSize() const { return sf::Vector2i(tileWidth, tileHeight); }

  const size_t Map::GetTilesetItemCount() const
  {
    return tileset.size();
  }

  const std::pair<bool, Map::Tile**> Map::LoadFromFile(Map& map, const std::string& path)
  {
    std::ifstream file(path.c_str());

    if (!file.is_open()) {
      file.close();
      return { false, nullptr };
    }

    std::string line;

    // name of map
    std::getline(file, line);
    std::string name = line;

    // rows x cols
    std::getline(file, line);
    size_t space_index = line.find_first_of(" ");

    std::string rows_str = line.substr(0, space_index);
    std::string cols_str = line.substr(space_index);
    unsigned rows = std::atoi(rows_str.c_str());
    unsigned cols = std::atoi(cols_str.c_str());

    Tile** tiles = new Tile * [rows] {};

    for (size_t i = 0; i < rows; i++) {
      tiles[i] = new Tile[cols]{};
    }

    size_t index = 0;

    // load all tiles
    while(std::getline(file, line))
    {
      std::stringstream linestream(line);
      std::string value;

      while (getline(linestream, value, ',') && static_cast<unsigned>(index) < rows*cols)
      {
        size_t row = index / cols;
        size_t col = index % cols;

        Tile& tile = tiles[row][col];

        bool is_integer = !value.empty() && std::find_if(value.begin(),
          value.end(), [](unsigned char c) { return !std::isdigit(c); }) == value.end();

        if (is_integer) {
          unsigned val = std::atoi(value.c_str());
          tile.ID = val;

          if (val != 0) {
            tile.solid = false;
          }
        }
        else {
          tile.ID = 1;
          tile.solid = false;
        }

        tile.token = value;

        index++;
      }
    }

    // default tileset code for now:
    auto texture = TEXTURES.LoadTextureFromFile("resources/ow/basic_tileset.png");
 
    std::vector<sf::Sprite> items;

    for (size_t index = 0; index < 3; index++) {
      sf::Sprite item;
      item.setTexture(*texture);
      item.setTextureRect(sf::IntRect{ 62 * static_cast<int>(index), 0, 62, 49 });
      item.setOrigin(sf::Vector2f((map.tileWidth * 0.5f) - 2.0f, 2.0f));
      items.push_back(item);
    }

    Map::Tileset tileset{};

    size_t i = 1;
    for (auto& item : items) {
      tileset.Register(i++, item);
    }

    map.Load(tileset, tiles, cols, rows);
    map.SetName(name);

    return { true, tiles };
  }

  std::pair<unsigned, unsigned> Map::PixelToRowCol(const sf::Vector2i& px) const
  {
    // convert it to world coordinates
    sf::Vector2f world = ENGINE.GetWindow()->mapPixelToCoords(px);

    // consider the point on screen relative to the camera focus
    auto pos = world - ENGINE.GetViewOffset();
    
    // respect the current scale and transform form isometric coordinates
    return IsoToRowCol({ pos.x / getScale().x, pos.y / getScale().y });
  }

  std::pair<unsigned, unsigned> Map::OrthoToRowCol(const sf::Vector2f& ortho) const
  {
    // divide by the tile size to get the integer grid values
    unsigned x = ortho.x / (tileWidth * 0.5f);
    unsigned y = ortho.y / tileHeight;

    return { y, x };
  }

  std::pair<unsigned, unsigned> Map::IsoToRowCol(const sf::Vector2f& iso) const
  {
    // convert from iso to ortho before converting to grid values
    return OrthoToRowCol(IsoToOrthogonal({ iso.x, iso.y }));
  }

  const std::string& Map::GetName() const
  {
    return name;
  }

  void Map::SetName(const std::string& name)
  {
    this->name = name;
  }
  const unsigned Map::GetCols() const
  {
    return this->cols;
  }
  const unsigned Map::GetRows() const
  {
    return this->rows;
  }

  // class Map::Tileset

  const sf::Sprite& Map::Tileset::Graphic(size_t ID) const
  {
    auto& item = idToSpriteHash.at(ID);
    return item.sprite;
  }

  void Map::Tileset::Register(size_t ID, const sf::Sprite& sprite)
  {
    Tileset::Item item{ sprite };
    idToSpriteHash.insert(std::make_pair(ID, item));
  }
  const size_t Map::Tileset::size() const
  {
    return idToSpriteHash.size();
  }
}


