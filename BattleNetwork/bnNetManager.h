#pragma once
#include <vector>
#include <map>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/IPAddress.h>
#include "bnIPacketProcessor.h"

class NetManager {
private:
  std::map<Poco::Net::SocketAddress, std::vector<std::shared_ptr<IPacketProcessor>>> handlers;
  std::map<std::shared_ptr<IPacketProcessor>, size_t> processorCounts;
  std::shared_ptr<Poco::Net::DatagramSocket> client; //!< us
  int myPort{};
public:
  NetManager();
  ~NetManager();

  void Update(double elapsed);
  void AddHandler(const Poco::Net::SocketAddress& sender, const std::shared_ptr<IPacketProcessor>& processor);
  void DropHandlers(const Poco::Net::SocketAddress& sender);
  void DropProcessor(const std::shared_ptr<IPacketProcessor>& processor);
  const bool BindPort(int port);
  Poco::Net::DatagramSocket& GetSocket();
  const std::string GetPublicIP();
};