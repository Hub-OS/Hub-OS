#pragma once

#include "bnPacketHeaders.h"
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <chrono>
#include <vector>

namespace Overworld
{
  class PacketShipper
  {
  private:
    struct BackedUpPacket
    {
      uint64_t id;
      std::chrono::time_point<std::chrono::steady_clock> creationTime;
      Poco::Buffer<char> data;
    };

    Poco::Net::SocketAddress socketAddress;
    uint64_t nextUnreliableSequenced;
    uint64_t nextReliable;
    uint64_t nextReliableOrdered;
    std::vector<BackedUpPacket> backedUpReliable;
    std::vector<BackedUpPacket> backedUpReliableOrdered;

    void acknowledgedReliable(uint64_t id);
    void acknowledgedReliableOrdered(uint64_t id);

  public:
    PacketShipper(Poco::Net::SocketAddress socketAddress);

    void Send(Poco::Net::DatagramSocket &socket, Reliability Reliability, const Poco::Buffer<char> body);
    void ResendBackedUpPackets(Poco::Net::DatagramSocket &socket);
    void Acknowledged(Reliability reliability, uint64_t id);
  };
} // namespace Overworld
