#pragma once

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <chrono>
#include <vector>

enum class Reliability : char
{
  Unreliable = 0,
  UnreliableSequenced,
  Reliable,
  ReliableSequenced,
  ReliableOrdered,
};

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
  bool failed;

  void sendSafe(Poco::Net::DatagramSocket& socket, const Poco::Buffer<char>& data);
  void acknowledgedReliable(uint64_t id);
  void acknowledgedReliableOrdered(uint64_t id);

public:
  PacketShipper(const Poco::Net::SocketAddress& socketAddress);

  bool HasFailed();
  void Send(Poco::Net::DatagramSocket& socket, Reliability Reliability, const Poco::Buffer<char>& body);
  void ResendBackedUpPackets(Poco::Net::DatagramSocket& socket);
  void Acknowledged(Reliability reliability, uint64_t id);
};
