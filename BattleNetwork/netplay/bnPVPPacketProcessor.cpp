#include "bnPVPPacketProcessor.h"
#include "bnNetPlaySignals.h"
#include "battlescene/bnNetworkBattleScene.h"

PVP::PacketProcessor::PacketProcessor(Poco::Net::SocketAddress& remoteAddress, NetworkBattleScene& scene) :
  remote(remoteAddress),
  scene(scene)
{
}

PVP::PacketProcessor::~PacketProcessor()
{
}

void PVP::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (sender != remote)
    return;

  if (read == 0)
    return;

  NetPlaySignals sig = *(NetPlaySignals*)buffer;
  size_t sigLen = sizeof(NetPlaySignals);
  Poco::Buffer<char> data{ 0 };
  data.append(buffer + sigLen, size_t(read) - sigLen);

  scene.processPacketBody(sig, data);

  lastPacketTime = std::chrono::steady_clock::now();
  errorCount = 0;
}

void PVP::PacketProcessor::Update(double elapsed) {
  constexpr int64_t MAX_ERROR_COUNT = 20;

  if (TimedOut() || errorCount > MAX_ERROR_COUNT) {
    scene.Quit(FadeOut::pixelate);
  }
}

void PVP::PacketProcessor::HandleError()
{
  errorCount++;
}

void PVP::PacketProcessor::SendPacket(const Poco::Buffer<char>& data)
{
  client->sendTo(data.begin(), data.size(), remote);
}

bool PVP::PacketProcessor::TimedOut() {
  auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - lastPacketTime
    );

  constexpr int64_t MAX_TIMEOUT_SECONDS = 5;

  return timeDifference.count() > MAX_TIMEOUT_SECONDS;
}