#pragma once
#include "../bnResourceHandle.h"
#include "bnOverworldMap.h"
#include <SFML/Graphics.hpp>
#include <memory>

namespace Overworld {
  class Minimap : public ResourceHandle, public sf::Transformable, public sf::Drawable {
  private:
    bool largeMapControls{};
    float scaling{}; //!< scaling used to fit everything on the screen
    sf::Vector2f offset{}; //!< screen offsets to align icons correctly
    sf::Vector2f panning{};
    std::string name;
    sf::RectangleShape rectangle;
    sf::Color bgColor{};
    SpriteProxyNode player, hp, warp, overlay, arrows, bakedMap;
    std::vector<std::shared_ptr<SpriteProxyNode>> warps;
    void DrawLayer(sf::RenderTarget& target, sf::RenderStates states, Map& map, size_t index);
    void EnforceTextureSizeLimits();
  public:
    static Minimap CreateFrom(const std::string& name, Map& map);
    Minimap();
    Minimap(const Minimap& rhs);
    ~Minimap();

    Minimap& operator=(const Minimap& rhs);

    void ResetPanning();
    void Pan(const sf::Vector2f& amount);
    void SetPlayerPosition(const sf::Vector2f& pos);
    void SetHomepagePosition(const sf::Vector2f& pos);
    void AddWarpPosition(const sf::Vector2f& pos);
    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override final;
  };
}