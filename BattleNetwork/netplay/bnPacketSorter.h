#pragma once

#include "bnPacketShipper.h"
#include "bnPacketAssembler.h"
#include "bnBufferReader.h"
#include "../bnLogger.h"
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <chrono>
#include <vector>

template<auto AckID>
class PacketSorter
{
private:
  struct BackedUpPacket
  {
    uint64_t id{};
    Poco::Buffer<char> data{ 0 };
  };

  Poco::Net::SocketAddress socketAddress;
  uint64_t nextReliable{};
  uint64_t nextUnreliableSequenced{};
  uint64_t nextReliableOrdered{};
  std::vector<uint64_t> missingReliable;
  std::vector<BackedUpPacket> backedUpOrderedPackets;
  std::chrono::time_point<std::chrono::steady_clock> lastMessageTime;
  PacketAssembler packetAssembler; //!< builds BigData packets

  uint64_t getExpectedId(Reliability reliability);
  void sendAck(Poco::Net::DatagramSocket& socket, Reliability reliability, uint64_t id);

public:
  PacketSorter(const Poco::Net::SocketAddress& socketAddress);

  std::chrono::time_point<std::chrono::steady_clock> GetLastMessageTime();
  std::vector<Poco::Buffer<char>> SortPacket(Poco::Net::DatagramSocket& socket, Poco::Buffer<char> packet);
};


template<auto AckID>
PacketSorter<AckID>::PacketSorter(const Poco::Net::SocketAddress& socketAddress)
{
  this->socketAddress = socketAddress;
  nextReliable = 0;
  nextUnreliableSequenced = 0;
  nextReliableOrdered = 0;
  lastMessageTime = std::chrono::steady_clock::now();
}

template<auto AckID>
std::chrono::time_point<std::chrono::steady_clock> PacketSorter<AckID>::GetLastMessageTime()
{
  return lastMessageTime;
}

template<auto AckID>
std::vector<Poco::Buffer<char>> PacketSorter<AckID>::SortPacket(
  Poco::Net::DatagramSocket& socket,
  Poco::Buffer<char> packet)
{
  BufferReader reader;

  Reliability reliability = reader.Read<Reliability>(packet);
  auto isPureUnreliable = reliability == Reliability::Unreliable;
  auto id = isPureUnreliable ? 0 : reader.Read<uint64_t>(packet);

  if (IsReliable(reliability) && getExpectedId(reliability) == 0 && id != 0) {
    // prevent trailing connections from leaking into new sorters
    // just ignore this packet, TODO: Handle UnreliableSequenced? not handling can eat packets
    return {};
  }

  auto dataOffset = reader.GetOffset();
  auto data = Poco::Buffer<char>(packet.begin() + dataOffset, packet.size() - dataOffset);

  lastMessageTime = std::chrono::steady_clock::now();
  std::vector<Poco::Buffer<char>> newPackets;

  switch (reliability)
  {
  case Reliability::Unreliable:
    return { data };
  case Reliability::UnreliableSequenced:
    if (id < nextUnreliableSequenced)
    {
      // ignore old packets
      return {};
    }

    nextUnreliableSequenced = id + 1;

    return { data };
  case Reliability::Reliable:
  case Reliability::BigData:
    sendAck(socket, reliability, id);

    if (id == nextReliable)
    {
      // expected
      nextReliable += 1;

      newPackets.push_back(data);
    }
    else if (id > nextReliable)
    {
      // skipped expected
      for (auto i = nextReliable; i < id; i++)
      {
        missingReliable.push_back(i);
      }

      nextReliable = id + 1;

      newPackets.push_back(data);
    }
    else
    {
      auto iterEnd = missingReliable.end();
      auto iter = std::find(missingReliable.begin(), missingReliable.end(), id);

      if (iter != iterEnd)
      {
        // one of the missing packets
        missingReliable.erase(iter);

        newPackets.push_back(data);
      }
    }

    if (reliability == Reliability::BigData && newPackets.size()) {
      // Prior `reliable` code checks for duplicates, so if we
      // arrive here, we have new data to read
      data = newPackets[0];

      size_t start_id{};
      size_t end_id{};
      size_t read{};

      std::memcpy(&start_id, data.begin(), sizeof(size_t));
      read += sizeof(size_t);

      std::memcpy(&end_id, data.begin() + read, sizeof(size_t));
      read += sizeof(size_t);

      Poco::Buffer<char> bigData(data.begin() + read, data.size() - read);

      packetAssembler.Process(start_id, end_id, id, bigData);
      newPackets = packetAssembler.Assemble();
    }

    // we already handled this packet
    return newPackets;
  case Reliability::ReliableOrdered:
    sendAck(socket, reliability, id);

    if (id == nextReliableOrdered)
    {
      auto i = 0;

      nextReliableOrdered += 1;

      for (auto& backedUpPacket : backedUpOrderedPackets)
      {
        if (backedUpPacket.id != nextReliableOrdered)
        {
          break;
        }

        nextReliableOrdered += 1;
        i += 1;
      }

      auto iterBegin = backedUpOrderedPackets.begin();

      // split backed up packets, store newer packets
      std::vector<BackedUpPacket> freedPackets(iterBegin, iterBegin + i);
      std::vector<BackedUpPacket> backedUpPackets(iterBegin + i, backedUpOrderedPackets.end());

      backedUpOrderedPackets = backedUpPackets;

      std::vector<Poco::Buffer<char>> packets{ data };

      for (auto& backedUpPacket : freedPackets)
      {
        packets.push_back(backedUpPacket.data);
      }

      return packets;
    }
    else if (id > nextReliableOrdered)
    {
      // sorted insert
      auto i = 0;
      auto shouldInsert = true;

      for (auto& backedUpPacket : backedUpOrderedPackets)
      {
        if (backedUpPacket.id == id)
        {
          shouldInsert = false;
          break;
        }
        if (backedUpPacket.id > id)
        {
          break;
        }
        i += 1;
      }

      if (shouldInsert)
      {
        backedUpOrderedPackets.emplace(
          backedUpOrderedPackets.begin() + i,
          BackedUpPacket{
              id,
              data,
          });
      }

      // can't use this packet until we recieve earlier packets
      // fall-through
    }

    // already handled
    return {};
  } // case ends

  Logger::Logf("%d", (int)reliability);
  // unreachable, all cases should be covered above
  Logger::Log("bnPacketSorter.h: How did we get here?");
  return {};
}

template<auto AckId>
uint64_t PacketSorter<AckId>::getExpectedId(Reliability reliability) {
  switch (reliability) {
  case Reliability::Unreliable:
    return 0;
  case Reliability::UnreliableSequenced:
    return nextUnreliableSequenced;
  case Reliability::Reliable:
    return nextReliable;
  case Reliability::ReliableSequenced:
    return nextUnreliableSequenced;
  case Reliability::ReliableOrdered:
    return nextReliableOrdered;
  case Reliability::BigData:
    return nextReliable;
  case Reliability::size:
    return 0;
  }

  return 0;
}

template<auto AckID>
void PacketSorter<AckID>::sendAck(Poco::Net::DatagramSocket& socket, Reliability reliability, uint64_t id)
{
  auto ackId = AckID;

  Poco::Buffer<char> data{ 0 };
  data.append((char)Reliability::Unreliable);
  data.append((char*)&ackId, sizeof(ackId));
  data.append((char)reliability);
  data.append((char*)&id, sizeof(id));

  try
  {
    socket.sendTo(data.begin(), (int)data.size(), socketAddress);
  }
  catch (Poco::IOException& e)
  {
    if (e.code() == POCO_EWOULDBLOCK) {
      return;
    }

    Logger::Logf("Sorter Network exception: %s", e.displayText().c_str());
  }
}
