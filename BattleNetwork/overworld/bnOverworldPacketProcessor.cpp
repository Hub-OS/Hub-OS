#include "bnOverworldPacketProcessor.h"
#include "bnOverworldPacketHeaders.h"
#include "../netplay/bnBufferReader.h"
#include "../netplay/bnBufferWriter.h"

constexpr double KEEP_ALIVE_RATE = 1.0f;

namespace Overworld {
  PacketProcessor::PacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxPayloadSize) :
    packetShipper(remoteAddress, maxPayloadSize),
    packetSorter(remoteAddress)
  {
    packetResendTimer = PACKET_RESEND_RATE;
    heartbeatTimer = KEEP_ALIVE_RATE;
  }

  void PacketProcessor::SetPacketBodyCallback(std::function<void(const Poco::Buffer<char>& data)> onPacketBody) {
    this->onPacketBody = onPacketBody;
  }

  void PacketProcessor::SetUpdateCallback(std::function<void(double elapsed)> onUpdate) {
    this->onUpdate = onUpdate;
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

  void PacketProcessor::Update(double elapsed) {
    packetResendTimer -= elapsed;

    if (packetResendTimer < 0) {
      packetShipper.ResendBackedUpPackets(*client);
      packetResendTimer = PACKET_RESEND_RATE;
    }

    if (background) {
      // only sending heartbeat in the background as we're constantly sending position in foreground
      heartbeatTimer -= elapsed;

      if (heartbeatTimer < 0) {
        BufferWriter writer;
        Poco::Buffer<char> buffer{ 0 };
        writer.Write(buffer, ClientEvents::heartbeat);

        packetShipper.Send(*client, heartbeatReliability, buffer);
        heartbeatTimer = PACKET_RESEND_RATE;
      }
    }

    if (onUpdate) {
      onUpdate(elapsed);
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
        else if(onPacketBody) {
          onPacketBody(data);
        }
        break;
      default:
        if(onPacketBody) {
          onPacketBody(data);
        }
      }
    }
  }
}
