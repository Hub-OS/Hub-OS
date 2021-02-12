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
    sf::Vector2f netWarpTilePos;
    unsigned int netWarpObjectId;

    void PingRemoteAreaServer();
    void EnableNetWarps(bool enabled);

  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    Homepage(swoosh::ActivityController&, bool guestAccount);

    void onUpdate(double elapsed) override;
    void onDraw(sf::RenderTexture& surface) override;
    void onStart() override;
    void onResume() override;
    void onLeave() override;

    void OnTileCollision() override;
  };
}