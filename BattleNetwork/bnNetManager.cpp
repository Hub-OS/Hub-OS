#include "bnNetManager.h"

NetManager::NetManager()
{

}

NetManager::~NetManager()
{
  for (auto& [key, val] : processors) {
    delete key;
  }
}

void NetManager::Update(double elapsed)
{
  for (auto& [key, val] : processors) {
    key->Update(elapsed);
  }
}

void NetManager::AddHandler(IPacketProcessor* processor, const Poco::Net::IPAddress& sender)
{
  auto& list = processors[processor];

  for (auto& addr : list) {
    if (addr == sender) {
      return;
    }
  }

  list.push_back(sender);
}

void NetManager::DropHandlers(const Poco::Net::IPAddress& sender)
{
  for (auto& [key, val] : processors) {
    auto iter = std::find(val.begin(), val.end(), sender);

    if (iter != val.end()) {
      iter = val.erase(iter);
    }
  }
}

const bool NetManager::BindPort(int port)
{
  try {
    Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), port);
    client = Poco::Net::DatagramSocket(sa);
    client.setBlocking(false);
  }
  catch (...) {
    return false;
  }

  return true;
}

Poco::Net::DatagramSocket& NetManager::GetSocket()
{
  return client;
}
