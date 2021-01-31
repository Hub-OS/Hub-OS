#pragma once
#include <Swoosh/Timer.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include "bnOverworldSceneBase.h"

namespace Overworld {
  class Homepage final : public SceneBase {
  private:
    bool scaledmap{ false }, clicked{ false };
    bool guest{ false }, infocus{ false };
    Poco::Net::DatagramSocket client; //!< us
    Poco::Net::SocketAddress remoteAddress; //!< server
    bool isConnected{ false }, reconnecting{ false };
    swoosh::Timer pingServerTimer;

    void PingRemoteAreaServer();

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
    void onResume() override;
    void onLeave() override;

    const std::pair<bool, Map::Tile**> FetchMapData() override;
    void OnTileCollision(const Map::Tile& tile) override;
  };
}