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
      size_t id;
      Poco::Buffer<char> data;
    };

    Poco::Net::SocketAddress socketAddress;
    size_t nextReliable;
    size_t nextUnreliableSequenced;
    size_t nextReliableOrdered;
    std::vector<size_t> missingReliable;
    std::vector<BackedUpPacket> backedUpOrderedPackets;

    void sendAck(Poco::Net::DatagramSocket &socket, Reliability Reliability, size_t id);

  public:
    PacketSorter(Poco::Net::SocketAddress socketAddress);

    std::vector<Poco::Buffer<char>> SortPacket(Poco::Net::DatagramSocket &socket, Poco::Buffer<char> packet);
  };
} // namespace Overworld
