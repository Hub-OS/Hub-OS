#include "bnMatchMakingPacketProcessor.h"
#include "bnMatchMakingScene.h"
#include "bnNetPlaySignals.h"
#include "bnBufferReader.h"

using namespace Poco;
using namespace Net;

MatchMaking::PacketProcessor::PacketProcessor(MatchMakingScene& scene) :
  scene(scene)
{
}

MatchMaking::PacketProcessor::~PacketProcessor()
{
}

void MatchMaking::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (read == 0)
    return;

  Poco::Buffer<char> packet{ 0 };
  packet.append(buffer, read);

  BufferReader reader;
  reader.Read<char>(packet);
  NetPlaySignals sig = reader.Read<NetPlaySignals>(packet);

  Poco::Buffer<char> data{ 0 };
  data.append(buffer + reader.GetOffset(), packet.size() - reader.GetOffset());

  scene.ProcessPacketBody(sig, data);
}

void MatchMaking::PacketProcessor::Update(double elapsed) {
}

void MatchMaking::PacketProcessor::SetNewRemote(const std::string& socketAddressStr)
{
  validRemote = true;

  try {
    // TODO: does this throw? 
    remoteAddr = Poco::Net::SocketAddress(socketAddressStr);
  }
  catch (...) {
    validRemote = false;
  }
}

void MatchMaking::PacketProcessor::SendPacket(const Poco::Buffer<char>& data)
{
  client->sendTo(data.begin(), (int)data.size(), remoteAddr);
}

const Poco::Net::SocketAddress& MatchMaking::PacketProcessor::GetRemoteAddr()
{
  return remoteAddr;
}

const bool MatchMaking::PacketProcessor::RemoteAddrIsValid() const
{
  return validRemote;
}
