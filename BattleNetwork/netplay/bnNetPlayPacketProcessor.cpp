#include "bnNetPlayPacketProcessor.h"

const float PACKET_RESEND_RATE = 1.0f / 20.f;

Netplay::PacketProcessor::PacketProcessor(const Poco::Net::SocketAddress& remoteAddress, uint16_t maxBytes) :
  remote(remoteAddress),
  packetShipper(remoteAddress, maxBytes),
  packetSorter(remoteAddress)
{

}

Netplay::PacketProcessor::~PacketProcessor()
{
}

void Netplay::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (read == 0)
    return;

  Poco::Buffer<char> packet{ 0 };
  packet.append(buffer, read);

  auto packetBodies = packetSorter.SortPacket(*client, packet);

  for (auto& data : packetBodies) {
    BufferReader reader;
    NetPlaySignals sig = reader.Read<NetPlaySignals>(data);

    if (sig == NetPlaySignals::ack) {
      Reliability reliability = reader.Read<Reliability>(data);
      uint64_t id = reader.Read<uint64_t>(data);
      packetShipper.Acknowledged(reliability, id);

      if (id == handshakeId && handshakeSent) {
        handshakeAck = true;
        Logger::Logf("Handshake acknowledge with reliability type %d", (int)reliability);
      }
    }
    else if(onPacketBodyCallback && !data.empty()) {
      constexpr auto sigSize = sizeof(NetPlaySignals);

      Poco::Buffer<char> body{ 0 };
      body.append(data.begin() + sigSize, data.size() - sigSize);

      onPacketBodyCallback(sig, body);
    }
  }

  lastPacketTime = std::chrono::steady_clock::now();
  errorCount = 0;
}

void Netplay::PacketProcessor::Update(double elapsed) {
  packetResendTimer -= (float)elapsed;

  if (packetResendTimer < 0) {
    packetShipper.ResendBackedUpPackets(*client);
    packetResendTimer = PACKET_RESEND_RATE;
  }

  // All this update loop does is kick for silence
  // If not enabled, return early
  if (!checkForSilence) return;

  constexpr int64_t MAX_ERROR_COUNT = 20;

  if ((TimedOut() || errorCount > MAX_ERROR_COUNT) && onKickCallback) {
    onKickCallback();
  }
}

void Netplay::PacketProcessor::UpdateHandshakeID(uint64_t id)
{
  handshakeId = id;
  handshakeAck = false;
  handshakeSent = true;
}

void Netplay::PacketProcessor::HandleError()
{
  errorCount++;
}

void Netplay::PacketProcessor::SetKickCallback(const decltype(onKickCallback)& callback)
{
  onKickCallback = callback;
}

void Netplay::PacketProcessor::SetPacketBodyCallback(const decltype(onPacketBodyCallback)& callback)
{
  onPacketBodyCallback = callback;
}

std::pair<Reliability, uint64_t> Netplay::PacketProcessor::SendPacket(Reliability reliability, const Poco::Buffer<char>& data)
{
  return packetShipper.Send(*client, reliability, data);
}

void Netplay::PacketProcessor::EnableKickForSilence(bool enabled)
{
  if (enabled && !checkForSilence) {
    // If we were not checking for silence before, then
    // make the window of time begin now
    lastPacketTime = std::chrono::steady_clock::now();
  }

  checkForSilence = enabled;
}

bool Netplay::PacketProcessor::IsHandshakeAck()
{
  return handshakeAck;
}

const double Netplay::PacketProcessor::GetAvgLatency() const
{
  return packetShipper.GetAvgLatency();
}

bool Netplay::PacketProcessor::TimedOut() {
  auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - lastPacketTime
    );

  constexpr int64_t MAX_TIMEOUT_SECONDS = 5;

  return timeDifference.count() > MAX_TIMEOUT_SECONDS;
}