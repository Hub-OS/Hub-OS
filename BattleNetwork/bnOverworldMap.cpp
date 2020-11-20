#include "bnOverworldMap.h"
#include "bnEngine.h"
#include <cmath>

namespace Overworld {
  Map::Map(int numOfCols, int numOfRows, int tileWidth, int tileHeight) : 
    cols(numOfCols), rows(numOfRows), tileWidth(tileWidth), tileHeight(tileHeight), 
    sf::Drawable(), sf::Transformable() {

    // We must have one for the origin
    sf::Uint8 lighten = 255;
    //lights.push_back(new Light(sf::Vector2f(0, 0), sf::Color(lighten, lighten, lighten, 255), 100));

    enableLighting = false;

    // std::cout << "num of lights: " << lights.size() << "\n";

    cam = nullptr;
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
    DeleteTiles();

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

  void Map::AddSprite(const SpriteProxyNode * _sprite)
  {
    sprites.push_back(_sprite);
  }

  void Map::RemoveSprite(const SpriteProxyNode * _sprite) {
    auto pos = std::find(sprites.begin(), sprites.end(), _sprite);

    if(pos != sprites.end())
      sprites.erase(pos);
  }

  void Map::Update(double elapsed)
  {
	  for(auto iter = map.begin(); iter != map.end(); ) {
		  if((*iter)->ShouldRemove()) {
			  delete *iter;
			  iter = map.erase(iter);
		  } else {
			  iter++;
		  }
	  }

    std::sort(sprites.begin(), sprites.end(), 
        [](const SpriteProxyNode* sprite, const SpriteProxyNode* other) 
        { return sprite->getPosition().y < other->getPosition().y; }
    );

  }

  void Map::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= this->getTransform();

    DrawTiles(target, states);
    DrawSprites(target, states);
  }

  void Map::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const {
    for (int i = 0; i < map.size(); i++) {
      sf::Sprite tileSprite(map[i]->GetTexture());
      auto iso = OrthoToIsometric(map[i]->GetPos());
      tileSprite.setPosition(iso);

      if (cam) {
        if (true || cam->IsInView(tileSprite)) {
          target.draw(tileSprite, states);
        }
      }
    }
  }

  void Map::DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const {
    for (int i = 0; i < sprites.size(); i++) {
      auto iso = OrthoToIsometric(sprites[i]->getPosition());
      sf::Sprite tileSprite(*sprites[i]->getTexture());
      tileSprite.setTextureRect(sprites[i]->getSprite().getTextureRect());
      tileSprite.setPosition(iso);
      tileSprite.setOrigin(sprites[i]->getOrigin());

      if (cam) {
        if (true || cam->IsInView(tileSprite)) {
          target.draw(tileSprite, states);
        }
      }
    }
  }
  const sf::Vector2f Map::OrthoToIsometric(sf::Vector2f ortho) const {
    sf::Vector2f iso{};
    iso.x = (ortho.x - ortho.y);
    iso.y = (ortho.x + ortho.y) * 0.5f;

    return iso;
  }

  const sf::Vector2f Map::IsoToOrthogonal(sf::Vector2f iso) const {
    sf::Vector2f ortho{};
    ortho.x = (2.0f * iso.y + iso.x) * 0.5f;
    ortho.y = (2.0f * iso.y - iso.x) * 0.5f;

    return ortho;
  }

  void Map::DeleteTiles() {
    for (int i = 0; i < map.size(); i++) {
      delete map[i];
    }

    map.clear();
  }

  const sf::Vector2i Map::GetTileSize() const { return sf::Vector2i(tileWidth, tileHeight); }


}
