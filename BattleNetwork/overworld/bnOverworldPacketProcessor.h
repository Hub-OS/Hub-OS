#pragma once
#include "bnOverworldPacketHeaders.h"
#include "../netplay/bnPacketShipper.h"
#include "../netplay/bnPacketSorter.h"
#include "../bnIPacketProcessor.h"
#include <Poco/Net/SocketAddress.h>
#include <optional>
#include <functional>

namespace Overworld {
  class PacketProcessor : public IPacketProcessor {
  public:
    PacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxPayloadSize);

    void SetPacketBodyCallback(std::function<void(const Poco::Buffer<char>& data)> onPacketBody);
    void SetUpdateCallback(std::function<void(double)> onUpdate);

    bool TimedOut();
    void SetBackground();
    void SetForeground();
    void SendPacket(Reliability reliability, Poco::Buffer<char> body);

    void Update(double elapsed) override;
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;

  private:
    std::function<void(const Poco::Buffer<char>& data)> onPacketBody;
    std::function<void(double)> onUpdate;
    PacketShipper packetShipper;
    PacketSorter<ClientEvents::ack> packetSorter;
    Reliability heartbeatReliability{};
    double heartbeatTimer{};
    double packetResendTimer{};
    bool background{};
    std::optional<Poco::Buffer<char>> latestMapBody;
  };
}
