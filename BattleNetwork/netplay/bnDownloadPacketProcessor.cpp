#include "bnDownloadPacketProcessor.h"
#include "bnDownloadScene.h"
#include "bnBufferReader.h"

using namespace Poco;
using namespace Net;

const float PACKET_RESEND_RATE = 1.0f / 20.f;

Download::PacketProcessor::PacketProcessor(const Poco::Net::SocketAddress& addr, DownloadScene& scene) :
  remoteAddr(addr),
  shipper(addr),
  sorter(addr),
  scene(scene)
{
}

Download::PacketProcessor::~PacketProcessor()
{
}

void Download::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (read == 0)
    return;

  Poco::Buffer<char> packet{ 0 };
  packet.append(buffer, read);

  auto packetBodies = sorter.SortPacket(*client, packet);

  for (auto data : packetBodies) {
    BufferReader reader;
    Download::PacketType sig = reader.Read<Download::PacketType>(data);

    if (sig == Download::PacketType::ack) {
      Reliability reliability = reader.Read<Reliability>(data);
      uint64_t id = reader.Read<uint64_t>(data);
      shipper.Acknowledged(reliability, id);
    }
    else {
      Poco::Buffer<char> body{ 0 };
      body.append(data.begin() + reader.GetOffset(), data.size() - reader.GetOffset());

      scene.ProcessPacketBody(sig, body);
    }
  }

  lastPacketTime = std::chrono::steady_clock::now();
  errorCount = 0;
}

void Download::PacketProcessor::Update(double elapsed) {
  packetResendTimer -= (float)elapsed;

  if (packetResendTimer < 0) {
    shipper.ResendBackedUpPackets(*client);
    packetResendTimer = PACKET_RESEND_RATE;
  }
}

void Download::PacketProcessor::SendPacket(Reliability reliability, const Poco::Buffer<char>& data)
{
  shipper.Send(*client, reliability, data);
}
