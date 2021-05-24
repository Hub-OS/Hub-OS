#pragma once

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <chrono>
#include <queue>
#include <map>
#include <array>
#include <vector>

enum class Reliability : char
{
  Unreliable = 0,
  UnreliableSequenced,
  Reliable,
  ReliableSequenced,
  ReliableOrdered,
  size
};

class PacketShipper
{
private:
  struct BackedUpPacket
  {
    uint64_t id{};
    std::chrono::time_point<std::chrono::steady_clock> creationTime;
    Poco::Buffer<char> data;
  };

  bool failed{};
  Poco::Net::SocketAddress socketAddress;
  uint64_t nextUnreliableSequenced{};
  uint64_t nextReliable{};
  uint64_t nextReliableOrdered{};
  std::vector<BackedUpPacket> backedUpReliable;
  std::vector<BackedUpPacket> backedUpReliableOrdered;
  std::array<std::map<uint64_t, std::chrono::time_point<std::chrono::steady_clock>>, static_cast<size_t>(Reliability::size)> packetStart;
  std::queue<std::chrono::microseconds> lagTime;
  std::chrono::microseconds avgLatency{}; // the average acknowledgement time

  void updateLagTime(Reliability type, uint64_t packetId);
  void sendSafe(Poco::Net::DatagramSocket& socket, const Poco::Buffer<char>& data);
  void acknowledgedReliable(uint64_t id);
  void acknowledgedReliableOrdered(uint64_t id);

public:
  PacketShipper(const Poco::Net::SocketAddress& socketAddress);

  bool HasFailed();
  std::pair<Reliability, uint64_t> Send(Poco::Net::DatagramSocket& socket, Reliability Reliability, const Poco::Buffer<char>& body);
  void ResendBackedUpPackets(Poco::Net::DatagramSocket& socket);
  void Acknowledged(Reliability reliability, uint64_t id);
  const std::chrono::microseconds GetAvgLatency() const;
};
