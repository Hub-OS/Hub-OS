#include "bnPacketSorter.h"

#include "bnBufferReader.h"
#include "../bnLogger.h"

Overworld::PacketSorter::PacketSorter(Poco::Net::SocketAddress socketAddress)
{
  this->socketAddress = socketAddress;
  nextReliable = 0;
  nextUnreliableSequenced = 0;
  nextReliableOrdered = 0;
  lastMessageTime = std::chrono::steady_clock::now();
}

std::chrono::time_point<std::chrono::steady_clock> Overworld::PacketSorter::GetLastMessageTime()
{
  return lastMessageTime;
}

std::vector<Poco::Buffer<char>> Overworld::PacketSorter::SortPacket(
  Poco::Net::DatagramSocket& socket,
  Poco::Buffer<char> packet)
{
  lastMessageTime = std::chrono::steady_clock::now();

  BufferReader reader;

  Reliability reliability = reader.Read<Reliability>(packet);
  auto isPureUnreliable = reliability == Reliability::Unreliable;
  auto id = isPureUnreliable ? 0 : reader.Read<uint64_t>(packet);
  auto dataOffset = reader.GetOffset();
  auto data = Poco::Buffer<char>(packet.begin() + dataOffset, packet.size() - dataOffset);

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
    sendAck(socket, reliability, id);

    if (id == nextReliable)
    {
      // expected
      nextReliable += 1;

      return { data };
    }
    else if (id > nextReliable)
    {
      // skipped expected
      for (auto i = nextReliable; i < id; i++)
      {
        missingReliable.push_back(i);
      }

      nextReliable = id + 1;

      return { data };
    }
    else
    {
      auto iterEnd = missingReliable.end();
      auto iter = std::find(missingReliable.begin(), missingReliable.end(), id);

      if (iter != iterEnd)
      {
        // one of the missing packets
        missingReliable.erase(iter);

        return { data };
      }
    }

    // we already handled this packet
    return {};
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
  }

  // unreachable, all cases should be covered above
  Logger::Log("bnPacketSorter.cpp: How did we get here?");
  return {};
}

void Overworld::PacketSorter::sendAck(Poco::Net::DatagramSocket& socket, Reliability reliability, uint64_t id)
{
  Poco::Buffer<char> data{ 0 };
  data.append((char)Reliability::Unreliable);

  ClientEvents clientEvent = ClientEvents::ack;
  data.append((char*)&clientEvent, sizeof(uint16_t));
  data.append((char)reliability);
  data.append((char*)&id, sizeof(id));

  socket.sendTo(data.begin(), (int)data.size(), socketAddress);
}
