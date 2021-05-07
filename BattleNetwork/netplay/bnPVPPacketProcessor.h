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
    unsigned errorCount{};
    std::chrono::time_point<std::chrono::steady_clock> lastPacketTime;
    NetworkBattleScene& scene;
    Poco::Net::SocketAddress& remote;
    PacketShipper packetShipper;
    PacketSorter<NetPlaySignals::ack> packetSorter;
    float packetResendTimer;
  public:
    PacketProcessor(Poco::Net::SocketAddress& remoteAddress, NetworkBattleScene& scene);
    ~PacketProcessor();
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;
    void Update(double elapsed) override;
    void HandleError();
    void SendPacket(Reliability reliability, const Poco::Buffer<char>& data);
    void EnableKickForSilence(bool enabled);
    bool TimedOut();
  };
}