#include "bnMatchMakingPacketProcessor.h"
#include "bnMatchMakingScene.h"
#include "../bnNetManager.h"
#include "bnNetPlaySignals.h"
#include "bnBufferReader.h"

using namespace Poco;
using namespace Net;

MatchMaking::PacketProcessor::PacketProcessor()
{
}

MatchMaking::PacketProcessor::~PacketProcessor()
{
}
void MatchMaking::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (RemoteAddrIsValid()) {
    proxy->OnPacket(buffer, read, sender);
  }
}

void MatchMaking::PacketProcessor::OnListen(const Poco::Net::SocketAddress& sender)
{
  if (proxy) {
    proxy->ShareSocket(this);
  }
}

void MatchMaking::PacketProcessor::Update(double elapsed) {
  if (RemoteAddrIsValid()) {
    proxy->Update(elapsed);
  }
}

void MatchMaking::PacketProcessor::SetNewRemote(const std::string& socketAddressStr, uint16_t maxBytes)
{
  validRemote = true;

  try {
    remote = Poco::Net::SocketAddress(socketAddressStr);
    proxy = std::make_shared<Netplay::PacketProcessor>(remote, maxBytes);
    proxy->SetPacketBodyCallback(callback);
    proxy->ShareSocket(this);
  }
  catch (...) {
    validRemote = false;
  }
}

const Poco::Net::SocketAddress& MatchMaking::PacketProcessor::GetRemoteAddr()
{
  return remote;
}

const bool MatchMaking::PacketProcessor::RemoteAddrIsValid() const
{
  return validRemote;
}

void MatchMaking::PacketProcessor::SendPacket(Reliability reliability, const Poco::Buffer<char>& data) {
  if (proxy) {
    proxy->SendPacket(reliability, data);
  }
}

void MatchMaking::PacketProcessor::SetKickCallback(const Netplay::PacketProcessor::KickFunc& callback)
{
}

void MatchMaking::PacketProcessor::SetPacketBodyCallback(const Netplay::PacketProcessor::PacketbodyFunc& callback)
{
  this->callback = callback;

  if (proxy) {
    proxy->SetPacketBodyCallback(callback);
  }
}

std::shared_ptr<Netplay::PacketProcessor> MatchMaking::PacketProcessor::GetProxy()
{
  return proxy;
}
