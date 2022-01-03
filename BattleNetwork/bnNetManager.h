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
  std::map<IPacketProcessor*, size_t> processorCounts;
  std::shared_ptr<Poco::Net::DatagramSocket> client; //!< us
  unsigned int myPort{};
  uint16_t maxPayloadSize{ DEFAULT_MAX_PAYLOAD_SIZE };
public:
  static const size_t LAG_WINDOW_LEN = 300;
  static const uint16_t DEFAULT_MAX_PAYLOAD_SIZE = 1300;

  NetManager();
  ~NetManager();

  void Update(double elapsed);
  void AddHandler(const Poco::Net::SocketAddress& sender, const std::shared_ptr<IPacketProcessor>& processor);
  void DropHandlers(const Poco::Net::SocketAddress& sender);
  void DropProcessor(const std::shared_ptr<IPacketProcessor>& processor);
  void DropProcessor(IPacketProcessor* processor);
  void SetMaxPayloadSize(uint16_t bytes);
  const uint16_t GetMaxPayloadSize() const;
  const bool BindPort(unsigned int port);
  Poco::Net::DatagramSocket& GetSocket();
  const std::string GetPublicIP();

  template<typename T>
  static const T CalculateLag(size_t packetCount, std::array<T, NetManager::LAG_WINDOW_LEN>& lagWindow, T next) {
    size_t window_len = std::min(packetCount, lagWindow.size());

    if (window_len == 0) {
      return T{ 0 };
    }

    T avg{ 0 };
    for (size_t i = 0; i < window_len; i++) {
      avg = avg + lagWindow[i];
    }

    if (next != 0.0) {
      avg = next + avg;
      window_len++;
    }

    avg = avg / static_cast<T>(window_len);

    return avg;
  }
};