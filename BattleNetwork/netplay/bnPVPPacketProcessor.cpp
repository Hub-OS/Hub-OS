#include "bnPVPPacketProcessor.h"
#include "bnNetPlaySignals.h"
#include "battlescene/bnNetworkBattleScene.h"

const float PACKET_RESEND_RATE = 1.0f / 20.f;

PVP::PacketProcessor::PacketProcessor(Poco::Net::SocketAddress& remoteAddress, NetworkBattleScene& scene) :
  remote(remoteAddress),
  packetShipper(remoteAddress),
  packetSorter(remoteAddress),
  scene(scene)
{
}

PVP::PacketProcessor::~PacketProcessor()
{
}

void PVP::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (read == 0)
    return;

  NetPlaySignals sig = *(NetPlaySignals*)buffer;
  size_t sigLen = sizeof(NetPlaySignals);
  Poco::Buffer<char> data{ 0 };
  data.append(buffer + sigLen, size_t(read) - sigLen);

  if (sig == NetPlaySignals::ack) {
    BufferReader reader;
    Reliability r = reader.Read<Reliability>(data);
    uint64_t id = reader.Read<uint64_t>(data);
    packetShipper.Acknowledged(r, id);
  }
  else {
    scene.processPacketBody(sig, data);
  }

  lastPacketTime = std::chrono::steady_clock::now();
  errorCount = 0;
}

void PVP::PacketProcessor::Update(double elapsed) {
  packetResendTimer -= elapsed;

  if (packetResendTimer < 0) {
    packetShipper.ResendBackedUpPackets(*client);
    packetResendTimer = PACKET_RESEND_RATE;
  }

  // All this update loop does is kick for silence
  // If not enabled, return early
  if (!checkForSilence) return;

  constexpr int64_t MAX_ERROR_COUNT = 20;

  if (TimedOut() || errorCount > MAX_ERROR_COUNT) {
    scene.Quit(FadeOut::pixelate);
  }
}

void PVP::PacketProcessor::HandleError()
{
  errorCount++;
}

void PVP::PacketProcessor::SendPacket(Reliability reliability, const Poco::Buffer<char>& data)
{
  packetShipper.Send(*client, reliability, data);
}

void PVP::PacketProcessor::EnableKickForSilence(bool enabled)
{
  if (enabled && !checkForSilence) {
    // If we were not checking for silence before, then
    // make the window of time begin now
    lastPacketTime = std::chrono::steady_clock::now();
  }

  checkForSilence = enabled;
}

bool PVP::PacketProcessor::TimedOut() {
  auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - lastPacketTime
    );

  constexpr int64_t MAX_TIMEOUT_SECONDS = 5;

  return timeDifference.count() > MAX_TIMEOUT_SECONDS;
}