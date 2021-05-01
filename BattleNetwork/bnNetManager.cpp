#include "bnNetManager.h"
#include "bnLogger.h"

NetManager::NetManager()
{
  client = std::make_shared<Poco::Net::DatagramSocket>();
  BindPort(0);
}

NetManager::~NetManager()
{
  // processors.clear() invoked by map dtor
}

constexpr int MAX_BUFFER_LEN = 65535;

void NetManager::Update(double elapsed)
{
  // TODO: REMOVE AFTER DEBUGGING PVP
  return;

  static char buffer[MAX_BUFFER_LEN] = { 0 };

  while (client->available()) {
    Poco::Net::SocketAddress sender;
    int read = client->receiveFrom(buffer, MAX_BUFFER_LEN, sender);

    auto it = handlers.find(sender);

    if (it == handlers.end()) {
      continue;
    }

    for (auto processor : handlers[sender]) {
      processor->OnPacket(buffer, read, sender);
    }
  }

  for (auto& [processor, _] : processorCounts) {
    processor->Update(elapsed);
  }
}

void NetManager::AddHandler(const Poco::Net::SocketAddress& sender, const std::shared_ptr<IPacketProcessor>& processor)
{
  auto& list = handlers[sender];

  for (auto& handler : list) {
    if (handler == processor) {
      return;
    }
  }

  list.push_back(processor);

  if (processorCounts.find(processor) == processorCounts.end()) {
    processor->SetSocket(client);
    processorCounts[processor] = 1;
  } else {
    processorCounts[processor] += 1;
  }
}

void NetManager::DropHandlers(const Poco::Net::SocketAddress& sender)
{
  for(auto& processor : handlers[sender]) {
    auto& count = processorCounts[processor];

    count -= 1;

    if(count == 0) {
      // erase processor so it no longer gets updated
      processorCounts.erase(processor);
    }
  }

  handlers.erase(sender);
}

void NetManager::DropProcessor(const std::shared_ptr<IPacketProcessor>& processor)
{
  auto countIter = processorCounts.find(processor);

  if (countIter == processorCounts.end()) {
    // processor was never added
    return;
  }

  auto& count = processorCounts[processor];

  for (auto& [_, processors] : handlers) {
    auto iter = std::find(processors.begin(), processors.end(), processor);

    if (iter != processors.end()) {
      iter = processors.erase(iter);
      count -= 1;
    }
  }

  if (count == 0) {
    // erase processor so it no longer gets updated
    processorCounts.erase(processor);
  }
}

const bool NetManager::BindPort(int port)
{
  try {
    Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), port);
    client->close();
    client->bind(sa, true);
    client->setBlocking(false);
  }
  catch (...) {
    return false;
  }

  return true;
}

Poco::Net::DatagramSocket& NetManager::GetSocket()
{
  return *client;
}
