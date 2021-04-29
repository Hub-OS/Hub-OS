#pragma once
#include <vector>
#include <map>
#include <Poco/Net/IPAddress.h>
#include <Poco/Net/DatagramSocket.h>
#include "bnIPacketProcessor.h"

class NetManager {
private:
  std::map<IPacketProcessor*, std::vector<Poco::Net::IPAddress>> processors;
  Poco::Net::DatagramSocket client; //!< us
  int myPort{};
public:
  NetManager();
  ~NetManager();

  void Update(double elapsed);
  void AddHandler(IPacketProcessor* processor, const Poco::Net::IPAddress& sender);
  void DropHandlers(const Poco::Net::IPAddress& sender);
  const bool BindPort(int port);
  Poco::Net::DatagramSocket& GetSocket();
};