#pragma once
#include <chrono>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/IPAddress.h>
#include "../bnIPacketProcessor.h"
#include "bnNetPlaySignals.h"

class MatchMakingScene;

namespace MatchMaking {
  class PacketProcessor : public IPacketProcessor {
  private:
    MatchMakingScene& scene;
    Poco::Net::SocketAddress remoteAddr;
    bool validRemote{};
  public:
    PacketProcessor(MatchMakingScene& scene);
    ~PacketProcessor();
    void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) override;
    void Update(double elapsed) override;
    void SetNewRemote(const std::string& socketAddressStr);
    void SendPacket(const Poco::Buffer<char>& data);
    const Poco::Net::SocketAddress& GetRemoteAddr();
    const bool RemoteAddrIsValid() const;
    const std::string GetPublicIP();
  };
}