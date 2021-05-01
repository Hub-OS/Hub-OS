#pragma once
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/IPAddress.h>
#include <Poco/Buffer.h>
#include <memory>

class IPacketProcessor {
protected:
  std::shared_ptr<Poco::Net::DatagramSocket> client;
private:
  void SetSocket(const std::shared_ptr<Poco::Net::DatagramSocket>& socket) {
    client = socket;
  }

  friend class NetManager;
public:
  virtual ~IPacketProcessor() { }
  virtual void OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) = 0;
  virtual void Update(double elapsed) = 0;
};
