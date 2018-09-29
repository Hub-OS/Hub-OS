#include "bnOverworldMap.h"
#include "bnEngine.h"

namespace Overworld {
  Map::Map(int numOfCols, int numOfRows, int tileWidth, int tileHeight) : cols(numOfCols), rows(numOfRows), tileWidth(tileWidth), tileHeight(tileHeight), sf::Drawable() {

    // We must have one for the origin
    sf::Uint8 lighten = 255;
    lights.push_back(new Light(sf::Vector2f(0, 0), sf::Color(lighten, lighten, lighten, 255), 100));

    enableLighting = true;

    std::cout << "num of lights: " << lights.size() << "\n";

    cam = nullptr;
  }

  void Map::ToggleLighting(bool state) {
    enableLighting = state;

    /*if (!enableLighting) {
      for (int i = 0; i < lights.size(); i++) {
        delete lights[i];
      }

      lights.clear();
    }*/
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

  void Map::AddSprite(sf::Sprite * _sprite)
  {
    sprites.push_back(_sprite);
  }

  void Map::RemoveSprite(sf::Sprite * _sprite) {
    auto pos = std::find(sprites.begin(), sprites.end(), _sprite);

    if(pos != sprites.end())
      sprites.erase(pos);
  }

  void Map::Update(double elapsed)
  {
    for (int i = 0; i < map.size(); i++) {
      if (map[i]->ShouldRemove()) {
        delete map[i];
        map.erase(map.begin() + i);
        i--;
      }
    }

    std::sort(sprites.begin(), sprites.end(), [](const sf::Sprite* sprite, const sf::Sprite* other) { return sprite->getPosition().y < other->getPosition().y; });

  }

  void Map::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    DrawTiles(target, states);
    DrawSprites(target, states);
  }

  void Map::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const {
    for (int i = 0; i < map.size(); i++) {
      sf::Sprite tileSprite(map[i]->GetTexture());

      if (enableLighting) {
        tileSprite.setColor(sf::Color::Black); // no lighting
      }

      tileSprite.setScale(2.0f, 2.0f);
      sf::Vector2f pos = map[i]->GetPos();

      if (cam) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;

        pos = OrthoToIsometric(pos*2.0f);

        tileSprite.setPosition(pos);

        if (!cam->IsInView(tileSprite)) {
          continue;
        }
      }

      pos = map[i]->GetPos();

      if (cam) {
        if (lights.size() > 0) {
          sf::View view = cam->GetView();
          sf::Vector2i posi = sf::Mouse::getPosition(*ENGINE.GetWindow());
          sf::Vector2f pos = sf::Vector2f((float)posi.x, (float)posi.y);
          lights[0]->SetPosition(IsoToOrthogonal(pos + (view.getCenter() - (view.getSize() / 8.0f))));
        }

        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;
      }

      for (int j = 0; j < lights.size() && enableLighting; j++) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        float deltaX = pos.x - (lights[j]->GetPosition().x - offset.x);
        float deltaY = pos.y - (lights[j]->GetPosition().y - offset.y);

        float deltaR = sqrt((float)(deltaX*deltaX + deltaY * deltaY));

        if (deltaR <= lights[j]->GetRadius()) {
          double dist = (lights[j]->GetRadius() - deltaR) / lights[j]->GetRadius();
          sf::Color c = tileSprite.getColor();

          double r = (dist*lights[j]->GetDiffuse().r) + c.r;
          double g = (dist*lights[j]->GetDiffuse().g) + c.g;
          double b = (dist*lights[j]->GetDiffuse().b) + c.b;

          r = std::min(255.0, r); g = std::min(255.0, g); b = std::min(255.0, b);

          sf::Color applied((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b, 255);
          tileSprite.setColor(applied);
        }
      }

      pos = OrthoToIsometric(pos*2.0f);

      if (cam) {
        tileSprite.setPosition(pos);

        if (cam->IsInView(tileSprite)) {
          target.draw(tileSprite, states);
        }
      }
      else {
        tileSprite.setPosition(pos);

        target.draw(tileSprite, states);
      }
    }

    if (cam) {
      for (int i = 0; i < lights.size() && enableLighting; i++) {
        sf::Sprite originTest(*TEXTURES.GetTexture(TextureType::LIGHT));

        sf::Vector2f pos = lights[i]->GetPosition();

        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;

        pos = OrthoToIsometric(pos*2.0f);

        originTest.setPosition(pos);
        target.draw(originTest);
      }
    }
  }

  void Map::DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const {
    for (int i = 0; i < sprites.size(); i++) {
      sf::Sprite tileSprite(*sprites[i]->getTexture());
      tileSprite.setTextureRect(sprites[i]->getTextureRect());
      tileSprite.setPosition(sprites[i]->getPosition());
      tileSprite.setOrigin(sprites[i]->getOrigin());

      if (enableLighting) {
        tileSprite.setColor(sf::Color::Black); // no lighting
      }

      tileSprite.setScale(2.0f, 2.0f);
      sf::Vector2f pos = sprites[i]->getPosition();

      if (cam) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;

        pos = OrthoToIsometric(pos*2.0f);

        tileSprite.setPosition(pos);

        if (!cam->IsInView(tileSprite)) {
          continue;
        }
      }

      pos = sprites[i]->getPosition();

      if (cam) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        pos -= offset;
      }

      for (int j = 0; j < lights.size() && enableLighting; j++) {
        sf::View view = cam->GetView();
        sf::Vector2f offset = IsoToOrthogonal(view.getCenter() - (view.getSize() / 2.0f));

        float deltaX = pos.x - (lights[j]->GetPosition().x - offset.x);
        float deltaY = pos.y - (lights[j]->GetPosition().y - offset.y);

        float deltaR = sqrt((float)(deltaX*deltaX + deltaY * deltaY));

        if (deltaR <= lights[j]->GetRadius()) {
          double dist = (lights[j]->GetRadius() - deltaR) / lights[j]->GetRadius();
          sf::Color c = tileSprite.getColor();

          double r = (dist*lights[j]->GetDiffuse().r) + c.r;
          double g = (dist*lights[j]->GetDiffuse().g) + c.g;
          double b = (dist*lights[j]->GetDiffuse().b) + c.b;

          r = std::min(255.0, r); g = std::min(255.0, g); b = std::min(255.0, b);

          sf::Color applied((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b, 255);
          tileSprite.setColor(applied);
        }
      }

      pos = OrthoToIsometric(pos*2.0f);

      if (cam) {
        tileSprite.setPosition(pos);

        if (cam->IsInView(tileSprite)) {
          target.draw(tileSprite, states);
        }
      }
      else {
        tileSprite.setPosition(pos);

        target.draw(tileSprite, states);
      }
    }
  }
  const sf::Vector2f Map::OrthoToIsometric(sf::Vector2f ortho) const {
    sf::Vector2f iso;
    float tileWidthHalf = (float)(tileWidth / 2);
    float tileHeightHalf = (float)(tileHeight / 2);

    iso.x = ((ortho.x / (float)tileWidth) - (ortho.y / (float)tileHeight)) * tileWidthHalf;
    iso.y = ((ortho.x / (float)tileWidth) + (ortho.y / (float)tileHeight)) * tileHeightHalf;

    return iso;
  }

  const sf::Vector2f Map::IsoToOrthogonal(sf::Vector2f iso) const {
    sf::Vector2f ortho;

    float tileWidthHalf = (float)(tileWidth / 2);
    float tileHeightHalf = (float)(tileHeight / 2);

    ortho.x = (iso.x / tileWidthHalf + iso.y / tileHeightHalf) / 2;
    ortho.y = (iso.y / tileHeightHalf - (iso.x / tileWidthHalf)) / 2;

    ortho.x *= tileWidth;
    ortho.y *= tileHeight;

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