#pragma once
#include "../bnResourceHandle.h"
#include "bnOverworldMap.h"
#include <SFML/Graphics.hpp>
#include <memory>

namespace Overworld {
  class Minimap : public ResourceHandle, public sf::Transformable, public sf::Drawable {
  private:
    bool validHpIcon{};
    float scaling{}; //!< scaling used to fit everything on the screen
    sf::Vector2f offset{}; //!< screen offsets to align icons correctly
    std::string name;
    sf::Color bgColor{};
    std::shared_ptr<sf::Texture> playerTex, hpTex, overlayTex, bakedMapTex;
    sf::Sprite player, hp, overlay, bakedMap;

    void DrawLayer(sf::RenderTarget& target, sf::RenderStates states, Map& map, size_t index);

  public:
    static Minimap CreateFrom(const std::string& name, Map& map);
    Minimap();
    ~Minimap();

    void SetPlayerPosition(const sf::Vector2f& pos);
    void SetHomepagePosition(const sf::Vector2f& pos);

    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override final;
  };
}