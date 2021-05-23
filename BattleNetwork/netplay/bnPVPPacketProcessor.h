#pragma once
#include <chrono>
#include "../bnIPacketProcessor.h"
#include "bnNetPlaySignals.h"
#include "bnPacketShipper.h"
#include "bnPacketSorter.h"

class NetworkBattleScene;

namespace PVP {
  class PacketProcessor : public IPacketProcessor {
  private:
    bool checkForSilence{}; //!< if true, processor kicks connection after lengthy silence
    bool handshakeComplete{};
    unsigned errorCount{};
    uint64_t handshakeId{}; //!< Latest handshake packet
    float packetResendTimer{};
    std::chrono::time_point<std::chrono::steady_clock> lastPacketTime;
    NetworkBattleScene& scene;
    Poco::Net::SocketAddress& remote;
    PacketShipper packetShipper;
    PacketSorter<NetPlaySignals::ack> packetSorter;
  public:
    PacketProcessor(Poco::Net::SocketAddress& remoteAddress, NetworkBattleScene& scene);
    ~PacketProcessor();
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;
    void Update(double elapsed) override;
    void UpdateHandshakeID(uint64_t id);
    void HandleError();
    std::pair<Reliability, uint64_t> SendPacket(Reliability reliability, const Poco::Buffer<char>& data);
    void EnableKickForSilence(bool enabled);
    bool TimedOut();
    bool IsHandshakeComplete();
    const std::chrono::microseconds GetAvgLatency() const;
  };
}