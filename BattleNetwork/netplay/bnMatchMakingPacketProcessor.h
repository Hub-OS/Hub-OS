#pragma once
#include <chrono>
#include "../bnIPacketProcessor.h"
#include "bnNetPlayPacketProcessor.h"

class MatchMakingScene;

namespace MatchMaking {
  class PacketProcessor : public IPacketProcessor {
  private:
    bool validRemote{false};
    Poco::Net::SocketAddress remote;
    std::shared_ptr<Netplay::PacketProcessor> proxy;
    Netplay::PacketProcessor::PacketbodyFunc callback;
  public:
    PacketProcessor();
    ~PacketProcessor();
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override final;
    void OnListen(const Poco::Net::SocketAddress& sender) override final;
    void Update(double elapsed) override final;
    void SetNewRemote(const std::string& socketAddressStr);
    const Poco::Net::SocketAddress& GetRemoteAddr();
    const bool RemoteAddrIsValid() const;
    void SendPacket(Reliability reliability, const Poco::Buffer<char>& data);
    void SetKickCallback(const Netplay::PacketProcessor::KickFunc& callback);
    void SetPacketBodyCallback(const Netplay::PacketProcessor::PacketbodyFunc& callback);
  };
}