#include "bnPacketShipper.h"

#include "../bnLogger.h"
#include <algorithm>

Overworld::PacketShipper::PacketShipper(Poco::Net::SocketAddress socketAddress)
{
  this->socketAddress = socketAddress;
  nextUnreliableSequenced = 0;
  nextReliable = 0;
  nextReliableOrdered = 0;
}

void Overworld::PacketShipper::Send(
    Poco::Net::DatagramSocket &socket,
    Reliability reliability,
    const Poco::Buffer<char> body)
{
  Poco::Buffer<char> data(0);

  switch (reliability)
  {
  case Reliability::Unreliable:
    data.append(0);
    data.append(body);

    socket.sendTo(data.begin(), (int)data.size(), socketAddress);
    break;
  // ignore old packets
  case Reliability::UnreliableSequenced:
    data.append(1);
    data.append((char *)&nextUnreliableSequenced, sizeof(nextUnreliableSequenced));
    data.append(body);

    socket.sendTo(data.begin(), (int)data.size(), socketAddress);

    nextUnreliableSequenced += 1;
    break;
  case Reliability::Reliable:
    data.append(2);
    data.append((char *)&nextReliable, sizeof(nextReliable));
    data.append(body);

    socket.sendTo(data.begin(), (int)data.size(), socketAddress);

    backedUpReliable.push_back(BackedUpPacket{
        .id = nextReliable,
        .creationTime = std::chrono::steady_clock::now(),
        .data = data,
    });

    nextReliable += 1;
    break;
  // stalls until packets arrive in order (if client gets packet 0 + 3 + 2, it processes 0, and waits for 1)
  case Reliability::ReliableOrdered:
    data.append(4);
    data.append((char *)&nextReliableOrdered, sizeof(nextReliableOrdered));
    data.append(body);

    socket.sendTo(data.begin(), (int)data.size(), socketAddress);

    backedUpReliableOrdered.push_back(BackedUpPacket{
        .id = nextReliableOrdered,
        .creationTime = std::chrono::steady_clock::now(),
        .data = data,
    });

    nextReliableOrdered += 1;
    break;
  }
}

void Overworld::PacketShipper::ResendBackedUpPackets(Poco::Net::DatagramSocket &socket)
{
  auto now = std::chrono::steady_clock::now();
  auto RETRY_DELAY = 1.0 / 20.0;

  for (auto &backedUpPacket : backedUpReliable)
  {
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - backedUpPacket.creationTime);
    if (duration.count() < RETRY_DELAY)
    {
      break;
    }

    auto data = backedUpPacket.data;
    socket.sendTo(data.begin(), (int)data.size(), socketAddress);
  }

  for (auto &backedUpPacket : backedUpReliableOrdered)
  {
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - backedUpPacket.creationTime);
    if (duration.count() < RETRY_DELAY)
    {
      break;
    }

    auto data = backedUpPacket.data;
    socket.sendTo(data.begin(), (int)data.size(), socketAddress);
  }
}

void Overworld::PacketShipper::Acknowledged(Reliability reliability, size_t id)
{
  switch (reliability)
  {
  case Reliability::Unreliable:
  case Reliability::UnreliableSequenced:
    Logger::Log("Server is acknowledging unreliable packets?");
    break;
  case Reliability::Reliable:
    acknowledgedReliable(id);
    break;
  case Reliability::ReliableOrdered:
    acknowledgedReliableOrdered(id);
    break;
  }
}

void Overworld::PacketShipper::acknowledgedReliable(size_t id)
{
  auto iterEnd = backedUpReliable.end();
  auto iter = std::find_if(backedUpReliable.begin(), backedUpReliable.end(), [=](BackedUpPacket &packet) { return packet.id == id; });

  if (iter == iterEnd)
  {
    return;
  }

  backedUpReliable.erase(iter);
}

void Overworld::PacketShipper::acknowledgedReliableOrdered(size_t id)
{
  auto iterEnd = backedUpReliableOrdered.end();
  auto iter = std::find_if(backedUpReliableOrdered.begin(), iterEnd, [=](BackedUpPacket &packet) { return packet.id == id; });

  if (iter == iterEnd)
  {
    return;
  }

  backedUpReliableOrdered.erase(iter);
}
