#pragma once
#include <functional>
#include <chrono>
#include "../bnIPacketProcessor.h"
#include "bnNetPlaySignals.h"
#include "bnPacketShipper.h"
#include "bnPacketSorter.h"

namespace Netplay {
  class PacketProcessor : public IPacketProcessor {
  private:
    bool checkForSilence{}; //!< if true, processor kicks connection after lengthy silence
    bool handshakeAck{}, handshakeSent{};
    unsigned errorCount{};
    uint64_t handshakeId{}; //!< Latest handshake packet
    float packetResendTimer{};
    std::chrono::time_point<std::chrono::steady_clock> lastPacketTime;
    Poco::Net::SocketAddress remote;
    PacketShipper packetShipper;
    PacketSorter<NetPlaySignals::ack> packetSorter;
    std::function<void()> onKickCallback;
    std::function<void(NetPlaySignals header, const Poco::Buffer<char>&)> onPacketBodyCallback;
  public:
    PacketProcessor(const Poco::Net::SocketAddress& remoteAddress);
    ~PacketProcessor();

    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;
    void Update(double elapsed) override;
    void UpdateHandshakeID(uint64_t id);
    void HandleError();
    void SetKickCallback(const decltype(onKickCallback)& callback);
    void SetPacketBodyCallback(const decltype(onPacketBodyCallback)& callback);
    std::pair<Reliability, uint64_t> SendPacket(Reliability reliability, const Poco::Buffer<char>& data);
    void EnableKickForSilence(bool enabled);
    bool TimedOut();
    bool IsHandshakeAck();
    const double GetAvgLatency() const;
  };
}