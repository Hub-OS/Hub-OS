#pragma once
#include "../bnResourceHandle.h"
#include "../bnDirection.h"
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
    SpriteProxyNode player, hp, warp, board, shop, overlay, arrows, conveyor, bakedMap;
    std::vector<std::shared_ptr<SpriteProxyNode>> markers;
    void EnforceTextureSizeLimits();
    void DrawLayer(sf::RenderTarget& target, sf::Shader& shader, sf::RenderStates states, Map& map, size_t index);
    void FindTileMarkers(Map& map);
    void FindObjectMarkers(Map& map);
    void AddMarker(const std::shared_ptr<SpriteProxyNode>& marker, const sf::Vector2f& pos, bool inShadow);
  public:
    static Minimap CreateFrom(const std::string& name, Map& map);
    Minimap();
    Minimap(const Minimap& rhs);
    ~Minimap();

    Minimap& operator=(const Minimap& rhs);

    void ResetPanning();
    void Pan(const sf::Vector2f& amount);
    void SetPlayerPosition(const sf::Vector2f& pos, bool isConcealed);
    void SetHomepagePosition(const sf::Vector2f& pos, bool isConcealed);
    void ClearIcons();
    void AddWarpPosition(const sf::Vector2f& pos, bool isConcealed);
    void AddShopPosition(const sf::Vector2f& pos, bool isConcealed);
    void AddBoardPosition(const sf::Vector2f& pos, bool flip, bool isConcealed);
    void AddConveyorPosition(const sf::Vector2f& pos, Direction direction, bool isConcealed);
    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override final;
  };
}