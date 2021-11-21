#include "bnPacketShipper.h"

#include "../bnLogger.h"
#include "../bnNetManager.h"
#include <Poco/Net/NetException.h>
#include <algorithm>

PacketShipper::PacketShipper(const Poco::Net::SocketAddress& socketAddress, uint16_t maxPayloadSize)
{
  this->socketAddress = socketAddress;
  this->maxPayloadSize = maxPayloadSize;
  nextUnreliableSequenced = 0;
  nextReliable = 0;
  nextReliableOrdered = 0;
  failed = false;
  lagWindow.fill(0);
}

bool PacketShipper::HasFailed() {
  return failed;
}

std::pair<Reliability, uint64_t> PacketShipper::Send(
  Poco::Net::DatagramSocket& socket,
  Reliability reliability,
  const Poco::Buffer<char>& body)
{
  Poco::Buffer<char> data(0);
  uint64_t newID{};

  switch (reliability)
  {
  case Reliability::Unreliable:
    data.append((int)Reliability::Unreliable);
    data.append(body);

    sendSafe(socket, data);
    break;
  // ignore old packets
  case Reliability::UnreliableSequenced:
    data.append((int)Reliability::UnreliableSequenced);
    data.append((char*)&nextUnreliableSequenced, sizeof(nextUnreliableSequenced));
    data.append(body);

    sendSafe(socket, data);

    newID = nextUnreliableSequenced;
    nextUnreliableSequenced += 1;
    break;
  case Reliability::Reliable:
    data.append((int)Reliability::Reliable);
    data.append((char*)&nextReliable, sizeof(nextReliable));
    data.append(body);

    sendSafe(socket, data);

    backedUpReliable.push_back(BackedUpPacket{
        nextReliable,
        std::chrono::steady_clock::now(),
        data
      });

    newID = nextReliable;
    nextReliable += 1;
    break;
  // stalls until packets arrive in order (if client gets packet 0 + 3 + 2, it processes 0, and waits for 1)
  case Reliability::ReliableOrdered:
    data.append((int)Reliability::ReliableOrdered);
    data.append((char*)&nextReliableOrdered, sizeof(nextReliableOrdered));
    data.append(body);

    sendSafe(socket, data);

    backedUpReliableOrdered.push_back(BackedUpPacket{
        nextReliableOrdered,
        std::chrono::steady_clock::now(),
        data
      });

    newID = nextReliableOrdered;
    nextReliableOrdered += 1;
    break;
  // (Specialized Reliability::Reliable) handles chunking big packets
  case Reliability::BigData:
    size_t bodySize = body.size();
    size_t headerSize = 1 + sizeof(nextReliable) + sizeof(size_t) * 2; // headers 1-4 below
    size_t maxChunkSize = maxPayloadSize - headerSize;

    size_t expectedChunks = bodySize / maxChunkSize;
    size_t remainder = bodySize % maxChunkSize;

    if (remainder > 0)
      expectedChunks++;

    uint64_t startId = nextReliable;
    uint64_t endId = startId + expectedChunks - 1;
    newID = startId;

    if (expectedChunks == 0) {
      Poco::Buffer<char> chunk{ 0 };
      chunk.append((char)Reliability::BigData); // header 1
      chunk.append((char*)&nextReliable, sizeof(nextReliable)); // header 2
      chunk.append((char*)&startId, sizeof(uint64_t)); // header 3
      chunk.append((char*)&endId, sizeof(uint64_t)); // header 4
      chunk.append(body.begin(), body.size());

      sendSafe(socket, chunk);

      backedUpReliable.push_back(BackedUpPacket{
        nextReliable,
        std::chrono::steady_clock::now(),
        chunk
        });

      nextReliable += 1;
    }
    else {
      size_t written = 0;

      for (size_t i = 0; i < expectedChunks; i++) {
        size_t chunkLength = maxChunkSize;

        if (remainder > 0 && i == expectedChunks - 1) {
          chunkLength = remainder;
        }

        Poco::Buffer<char> chunk{ 0 };
        chunk.append((int)Reliability::BigData); // header 1
        chunk.append((char*)&nextReliable, sizeof(nextReliable)); // header 2
        chunk.append((char*)&startId, sizeof(uint64_t)); // header 3
        chunk.append((char*)&endId, sizeof(uint64_t)); // header 4
        chunk.append(body.begin() + written, chunkLength);
        written += chunkLength;

        sendSafe(socket, chunk);

        backedUpReliable.push_back(BackedUpPacket{
          nextReliable,
          std::chrono::steady_clock::now(),
          chunk
          });

        nextReliable += 1;
      }
    }
    // end case
    break;
  }

  if (IsReliable) {
    size_t index = static_cast<size_t>(reliability);
    packetStart[index][newID] = std::chrono::steady_clock::now();
  }

  return { reliability, newID };
}

void PacketShipper::updateLagTime(Reliability type, uint64_t packetId)
{
  size_t index = static_cast<size_t>(type);
  auto& packetStartForType = packetStart[index];
  auto iter = packetStartForType.find(packetId);

  if (iter != packetStartForType.end()) {
    auto start = iter->second;
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    avgLatency = NetManager::CalculateLag(ackPackets, lagWindow, (double)(duration.count()));
    ackPackets++;
    lagWindow[ackPackets % NetManager::LAG_WINDOW_LEN] = (double)duration.count();

    packetStartForType.erase(iter);
  }
}

void PacketShipper::ResendBackedUpPackets(Poco::Net::DatagramSocket& socket)
{
  auto now = std::chrono::steady_clock::now();
  auto RETRY_DELAY = 1.0 / 20.0;

  for (auto& backedUpPacket : backedUpReliable)
  {
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - backedUpPacket.creationTime);
    if (duration.count() < RETRY_DELAY) {
      break;
    }

    auto data = backedUpPacket.data;
    sendSafe(socket, data);
  }

  for (auto& backedUpPacket : backedUpReliableOrdered)
  {
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - backedUpPacket.creationTime);
    if (duration.count() < RETRY_DELAY) {
      break;
    }

    auto data = backedUpPacket.data;
    sendSafe(socket, data);
  }
}

void PacketShipper::sendSafe(
  Poco::Net::DatagramSocket& socket,
  const Poco::Buffer<char>& data)
{
  try
  {
    socket.sendTo(data.begin(), (int)data.size(), socketAddress);
  }
  catch (Poco::IOException& e)
  {
    if (e.code() == POCO_EWOULDBLOCK) {
      return;
    }
    Logger::Logf("Shipper Network exception: %s", e.displayText().c_str());

    failed = true;
  }
}

void PacketShipper::Acknowledged(Reliability reliability, uint64_t id)
{
  switch (reliability)
  {
  case Reliability::Unreliable:
  case Reliability::UnreliableSequenced:
    Logger::Logf("Server is acknowledging unreliable packets? ID: %i", id);
    break;
  case Reliability::Reliable:
  case Reliability::BigData:
    acknowledgedReliable(reliability, id);
    break;
  case Reliability::ReliableOrdered:
    acknowledgedReliableOrdered(id);
    break;
  }
}

const double PacketShipper::GetAvgLatency() const
{
  return avgLatency / 2.f; // ack is a round trip, so we need half the time to arrive
}

void PacketShipper::acknowledgedReliable(Reliability type, uint64_t id)
{
  auto iterEnd = backedUpReliable.end();
  auto iter = std::find_if(backedUpReliable.begin(), backedUpReliable.end(), [=](BackedUpPacket& packet) { return packet.id == id; });

  if (iter == iterEnd) {
    return;
  }

  updateLagTime(type, id); // type can be Reliability::Reliable, or Reliability::BigData
  backedUpReliable.erase(iter);
}

void PacketShipper::acknowledgedReliableOrdered(uint64_t id)
{
  auto iterEnd = backedUpReliableOrdered.end();
  auto iter = std::find_if(backedUpReliableOrdered.begin(), iterEnd, [=](BackedUpPacket& packet) { return packet.id == id; });

  if (iter == iterEnd) {
    return;
  }

  updateLagTime(Reliability::ReliableOrdered, id);
  backedUpReliableOrdered.erase(iter);
}
