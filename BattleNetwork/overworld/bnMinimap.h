#pragma once
#include "../bnResourceHandle.h"
#include "../bnDirection.h"
#include "bnOverworldMap.h"
#include "bnOverworldMenu.h"
#include <SFML/Graphics.hpp>
#include <memory>

namespace Overworld {
  class Minimap : public Menu, public sf::Transformable, public ResourceHandle {
  private:
    bool largeMapControls{};
    float scaling{}; //!< scaling used to fit everything on the screen
    sf::Vector2f offset{}; //!< screen offsets to align icons correctly
    sf::Vector2f panning{};
    std::string name;
    sf::RectangleShape rectangle;
    sf::Color bgColor{};
    SpriteProxyNode player, home, warp, board, shop, conveyor, arrow, overlay, overlayArrows, bakedMap;
    std::vector<std::shared_ptr<SpriteProxyNode>> playerMarkers;
    std::vector<std::shared_ptr<SpriteProxyNode>> mapMarkers;
    void DrawLayer(sf::RenderTarget& target, sf::Shader& shader, sf::RenderStates states, Map& map, size_t index);
    void FindMapMarkers(Map& map);
    void FindTileMarkers(Map& map);
    void FindObjectMarkers(Map& map);
    void AddMarker(const std::shared_ptr<SpriteProxyNode>& marker, const sf::Vector2f& pos, bool inShadow);
  public:
    class PlayerMarker : public SpriteProxyNode {
    private:
      sf::Color markerColor;
    public:
      PlayerMarker(sf::Color color) : markerColor(color) {};

      sf::Color GetMarkerColor();
      void SetMarkerColor(sf::Color);
    };

    Minimap();
    Minimap(const Minimap& rhs) = delete;
    ~Minimap();

    void Update(const std::string& name, Map& map);
    void ResetPanning();
    void Pan(const sf::Vector2f& amount);
    void SetPlayerPosition(const sf::Vector2f& pos, bool isConcealed);
    void AddPlayerMarker(std::shared_ptr<PlayerMarker> marker);
    void UpdatePlayerMarker(PlayerMarker& playerMarker, sf::Vector2f pos, bool isConcealed);
    void RemovePlayerMarker(std::shared_ptr<PlayerMarker> playerMarker);
    void ClearIcons();
    void AddHomepagePosition(const sf::Vector2f& pos, bool isConcealed);
    void AddWarpPosition(const sf::Vector2f& pos, bool isConcealed);
    void AddShopPosition(const sf::Vector2f& pos, bool isConcealed);
    void AddBoardPosition(const sf::Vector2f& pos, bool flip, bool isConcealed);
    void AddConveyorPosition(const sf::Vector2f& pos, Direction direction, bool isConcealed);
    void AddArrowPosition(const sf::Vector2f& pos, Direction direction);
    bool IsFullscreen() override { return true; };
    void Open() override;
    void HandleInput(InputManager& input, sf::Vector2f mousePos) override;
    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override final;
  };
}