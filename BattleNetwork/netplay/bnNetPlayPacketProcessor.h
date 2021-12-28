#pragma once
#include <functional>
#include <chrono>
#include "../bnIPacketProcessor.h"
#include "bnNetPlaySignals.h"
#include "bnPacketShipper.h"
#include "bnPacketSorter.h"

namespace Netplay {
  class PacketProcessor : public IPacketProcessor {
  public:
    using KickFunc = std::function<void()>;
    using PacketbodyFunc = std::function<void(NetPlaySignals, const Poco::Buffer<char>&)>;

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
    KickFunc onKickCallback;
    PacketbodyFunc onPacketBodyCallback;
    std::vector<Poco::Buffer<char>> pendingPackets;

    void ProcessPackets(std::vector<Poco::Buffer<char>> packets);
  public:
    PacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxBytes);
    virtual ~PacketProcessor();

    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override final;
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