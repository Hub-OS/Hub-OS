#include "bnOverworldPacketProcessor.h"
#include "bnPacketHeaders.h"
#include "../netplay/bnBufferReader.h"

constexpr double KEEP_ALIVE_RATE = 1.0f;

namespace Overworld {
  PacketProcessor::PacketProcessor(const Poco::Net::SocketAddress& remoteAddress, std::function<void(const Poco::Buffer<char>& data)> onPacketBody) :
    packetShipper(remoteAddress),
    packetSorter(remoteAddress),
    onPacketBody(onPacketBody)
  {
    packetResendTimer = PACKET_RESEND_RATE;
    keepAliveTimer = KEEP_ALIVE_RATE;
  }

  bool PacketProcessor::TimedOut() {
    auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - packetSorter.GetLastMessageTime()
      );

    constexpr int64_t MAX_TIMEOUT_SECONDS = 5;

    return timeDifference.count() > MAX_TIMEOUT_SECONDS;
  }

  void PacketProcessor::SetBackground() {
    background = true;
  }

  void PacketProcessor::SetForeground() {
    background = false;

    if (latestMapBody) {
      onPacketBody(latestMapBody.value());
      latestMapBody = {};
    }
  }

  void PacketProcessor::SendPacket(Reliability reliability, Poco::Buffer<char> body) {
    packetShipper.Send(*client, reliability, body);
  }

  void PacketProcessor::SendKeepAlivePacket(Reliability reliability, Poco::Buffer<char> body) {
    packetShipper.Send(*client, reliability, body);

    // store packet to send when in background
    keepAliveBody = body;
    keepAliveReliability = reliability;
  }

  void PacketProcessor::Update(double elapsed) {
    packetResendTimer -= elapsed;

    if (packetResendTimer < 0) {
      packetShipper.ResendBackedUpPackets(*client);
      packetResendTimer = PACKET_RESEND_RATE;
    }

    if (background) {
      keepAliveTimer -= elapsed;

      if (keepAliveTimer < 0) {
        packetShipper.Send(*client, keepAliveReliability, keepAliveBody);
        keepAliveTimer = PACKET_RESEND_RATE;
      }
    }
  }

  void PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
    Poco::Buffer<char> packet{ 0 };
    packet.append(buffer, size_t(read));

    auto packetBodies = packetSorter.SortPacket(*client, packet);

    for (auto& data : packetBodies) {
      BufferReader reader;

      auto sig = reader.Read<ServerEvents>(data);

      switch (sig) {
      case ServerEvents::ack:
      {
        Reliability r = reader.Read<Reliability>(data);
        uint64_t id = reader.Read<uint64_t>(data);
        packetShipper.Acknowledged(r, id);
        break;
      }
      case ServerEvents::map:
        if (background) {
          // processing the map is pretty heavy
          latestMapBody = data;
        }
        else {
          onPacketBody(data);
        }
        break;
      default:
        onPacketBody(data);
      }
    }
  }
}
