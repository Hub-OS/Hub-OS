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
      size_t id;
      std::chrono::time_point<std::chrono::steady_clock> creationTime;
      Poco::Buffer<char> data;
    };

    Poco::Net::SocketAddress socketAddress;
    size_t nextUnreliableSequenced;
    size_t nextReliable;
    size_t nextReliableOrdered;
    std::vector<BackedUpPacket> backedUpReliable;
    std::vector<BackedUpPacket> backedUpReliableOrdered;

    void acknowledgedReliable(size_t id);
    void acknowledgedReliableOrdered(size_t id);

  public:
    PacketShipper(Poco::Net::SocketAddress socketAddress);

    void Send(Poco::Net::DatagramSocket &socket, Reliability Reliability, const Poco::Buffer<char> body);
    void ResendBackedUpPackets(Poco::Net::DatagramSocket &socket);
    void Acknowledged(Reliability reliability, size_t id);
  };
} // namespace Overworld
