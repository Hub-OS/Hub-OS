#pragma once

#include "bnPacketHeaders.h"
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <chrono>
#include <vector>

namespace Overworld
{
  class PacketSorter
  {
  private:
    struct BackedUpPacket
    {
      uint64_t id;
      Poco::Buffer<char> data;
    };

    Poco::Net::SocketAddress socketAddress;
    uint64_t nextReliable;
    uint64_t nextUnreliableSequenced;
    uint64_t nextReliableOrdered;
    std::vector<uint64_t> missingReliable;
    std::vector<BackedUpPacket> backedUpOrderedPackets;

    void sendAck(Poco::Net::DatagramSocket &socket, Reliability Reliability, uint64_t id);

  public:
    PacketSorter(Poco::Net::SocketAddress socketAddress);

    std::vector<Poco::Buffer<char>> SortPacket(Poco::Net::DatagramSocket &socket, Poco::Buffer<char> packet);
  };
} // namespace Overworld
