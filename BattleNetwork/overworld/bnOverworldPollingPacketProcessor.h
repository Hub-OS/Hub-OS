#pragma once

#include "bnOverworldPacketHeaders.h"
#include "../netplay/bnPacketShipper.h"
#include "../bnIPacketProcessor.h"
#include <Swoosh/Timer.h>
#include <Poco/Net/SocketAddress.h>
#include <chrono>
#include <functional>

namespace Overworld {
  enum class ServerStatus {
    older_version,
    newer_version,
    offline,
    online
  };

  class PollingPacketProcessor : public IPacketProcessor {
  public:
    PollingPacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxPayloadSize, std::function<void(ServerStatus, uint16_t)> onPacketBody);

    bool TimedOut();
    void Update(double elapsed) override;
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;

  private:
    std::function<void(ServerStatus, uint16_t)> onResolve;
    PacketShipper packetShipper;
    swoosh::Timer pingServerTimer;
    std::chrono::time_point<std::chrono::steady_clock> lastMessageTime;
  };
}
