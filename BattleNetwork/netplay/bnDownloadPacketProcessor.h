#pragma once
#include <chrono>
#include "../bnIPacketProcessor.h"
#include "bnPacketShipper.h"
#include "bnPacketSorter.h"

class DownloadScene;

namespace Download {
  enum class PacketType : char {
    card_list = 100, // set to 100+ to prevent overlap with NetPlay packets (maybe just use NetPlay packets?)
    custom_character,
    ack
  };

  class PacketProcessor : public IPacketProcessor {
  private:
    DownloadScene& scene;
    Poco::Net::SocketAddress remoteAddr;
    PacketShipper shipper;
    PacketSorter<Download::PacketType::ack> sorter;

    bool checkForSilence{}; //!< if true, processor kicks connection after lengthy silence
    unsigned errorCount{};
    float packetResendTimer{};
    std::chrono::time_point<std::chrono::steady_clock> lastPacketTime;

  public:
    PacketProcessor(const Poco::Net::SocketAddress& addr, DownloadScene& scene);
    ~PacketProcessor();
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;
    void Update(double elapsed) override;
    void SendPacket(Reliability reliability, const Poco::Buffer<char>& data);
  };
}