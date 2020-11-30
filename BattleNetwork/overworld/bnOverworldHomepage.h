#pragma once
#include "bnOverworldSceneBase.h"

namespace Overworld {
  class Homepage final : public SceneBase {
  private:
    bool scaledmap{ false }, clicked{ false };
    bool guest{ false };

  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    Homepage(swoosh::ActivityController&, bool guestAccount);

    /**
    * @brief deconstructor
    */
    ~Homepage();

    void onUpdate(double elapsed) override;
    void onDraw(sf::RenderTexture& surface) override;
    void onStart() override;

    const std::pair<bool, Map::Tile**> FetchMapData() override;
    void OnTileCollision(const Map::Tile& tile) override;
  };
}