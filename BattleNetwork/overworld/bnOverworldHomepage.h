#pragma once
#include <Swoosh/Timer.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include "bnOverworldSceneBase.h"

namespace Overworld {
  class Homepage final : public SceneBase {
  private:
    enum class CyberworldStatus {
      mismatched_version,
      offline,
      online
    };

    bool scaledmap{ false }, clicked{ false };
    bool guest{ false }, infocus{ false };
    Poco::Net::SocketAddress remoteAddress; //!< server
    uint16_t maxPayloadSize{};
    bool reconnecting{ false };
    swoosh::Timer pingServerTimer;
    sf::Vector3f netWarpTilePos;
    unsigned int netWarpObjectId{};
    CyberworldStatus cyberworldStatus{ CyberworldStatus::offline };

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
    void OnInteract() override;
  };
}