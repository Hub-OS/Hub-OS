#pragma once
#include <Swoosh/Timer.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include "bnOverworldSceneBase.h"
#include "bnOverworldPacketProcessor.h"

namespace Overworld {
  class Homepage final : public SceneBase {
  private:
    enum class CyberworldStatus {
      mismatched_version,
      offline,
      online
    };

    bool scaledmap{ false }, clicked{ false };
    bool infocus{ false };
    Poco::Net::SocketAddress remoteAddress; //!< server
    std::shared_ptr<PacketProcessor> packetProcessor;
    uint16_t maxPayloadSize{};
    swoosh::Timer pingServerTimer;
    sf::Vector3f netWarpTilePos;
    unsigned int netWarpObjectId{};
    CyberworldStatus cyberworldStatus{ CyberworldStatus::offline };

    void PingRemoteAreaServer();
    void ProcessPacketBody(const Poco::Buffer<char>& buffer);
    void EnableNetWarps(bool enabled);

  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    Homepage(swoosh::ActivityController&);
    ~Homepage();

    void onUpdate(double elapsed) override;
    void onDraw(sf::RenderTexture& surface) override;
    void onStart() override;
    void onEnd() override;
    void onResume() override;
    void onLeave() override;

    void OnTileCollision() override;
    void OnInteract() override;
  };
}