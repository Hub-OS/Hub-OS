#include "bnOverworldPollingPacketProcessor.h"

#include "bnPacketHeaders.h"
#include "../netplay/bnBufferWriter.h"
#include "../netplay/bnBufferReader.h"
#include <optional>

constexpr sf::Int32 PING_SERVER_MILI = 1000;

namespace Overworld {
  PollingPacketProcessor::PollingPacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxPayloadSize, std::function<void(ServerStatus, uint16_t)> onResolve) :
    packetShipper(remoteAddress, maxPayloadSize),
    onResolve(onResolve)
  {
    pingServerTimer.reverse(true);
    pingServerTimer.set(sf::milliseconds(PING_SERVER_MILI));
    pingServerTimer.start();
  }

  bool PollingPacketProcessor::TimedOut() {
    auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - lastMessageTime
      );

    constexpr int64_t MAX_TIMEOUT_SECONDS = 5;

    return timeDifference.count() > MAX_TIMEOUT_SECONDS;
  }

  void PollingPacketProcessor::Update(double elapsed) {
    pingServerTimer.update(sf::seconds(static_cast<float>(elapsed)));

    if (pingServerTimer.getElapsed().asSeconds() == 0.0f) {
      Poco::Buffer<char> buffer{ 0 };
      BufferWriter writer;
      writer.Write(buffer, ClientEvents::ping);

      packetShipper.Send(*client, Reliability::Unreliable, buffer);

      pingServerTimer.set(sf::milliseconds(PING_SERVER_MILI));
    }

    if (TimedOut()) {
      // set last message time to now to prevent resolve spam
      lastMessageTime = std::chrono::steady_clock::now();
      onResolve(ServerStatus::offline, 0);
    }
  }

  void PollingPacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
    BufferReader reader;
    auto data = Poco::Buffer<char>(buffer, read);
    lastMessageTime = std::chrono::steady_clock::now();

    if (reader.Read<Reliability>(data) != Reliability::Unreliable) {
      // ignoring, might be for a previous processor
      return;
    }

    if (reader.Read<ServerEvents>(data) != ServerEvents::pong) {
      // ignoring, might be for a previous processor
      return;
    }

    auto serverBranch = reader.ReadString<uint16_t>(data);

    if (serverBranch != VERSION_ID) {
      onResolve(ServerStatus::offline, 0);
      return;
    }
    auto serverIteration = reader.Read<uint64_t>(data);

    if (VERSION_ITERATION < serverIteration) {
      onResolve(ServerStatus::newer_version, 0);
      return;
    }

    if (VERSION_ITERATION > serverIteration) {
      onResolve(ServerStatus::older_version, 0);
      return;
    }

    auto serverMaxPayloadSize = reader.Read<uint16_t>(data);
    onResolve(ServerStatus::online, serverMaxPayloadSize);
  }
}
