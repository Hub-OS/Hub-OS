#include "bnNetManager.h"
#include "bnLogger.h"
#include <array>

using namespace Poco;
using namespace Net;

NetManager::NetManager()
{
  client = std::make_shared<Poco::Net::DatagramSocket>();
  BindPort(0);
}

NetManager::~NetManager()
{
  // `processors.clear()` is invoked by map dtor
}

constexpr int MAX_BUFFER_LEN = 65535;

void NetManager::Update(double elapsed)
{
  static char buffer[MAX_BUFFER_LEN] = { 0 };

  while (client->available()) {
    Poco::Net::SocketAddress sender;

    try {
      int read = client->receiveFrom(buffer, MAX_BUFFER_LEN, sender);

      auto it = handlers.find(sender);

      if (it == handlers.end()) {
        continue;
      }

      // make a copy as a processor may drop in here 
      auto matchingProcessors = it->second;

      for (auto processor : matchingProcessors) {
        processor->OnPacket(buffer, read, sender);
      }
    }
    catch (Poco::Exception& e) {
      Logger::Logf(LogLevel::critical, "NetManager exception: %s", e.what());
    }
  }

  size_t size = processorCounts.size();
  for (auto iter = processorCounts.begin(); iter != processorCounts.end(); iter++) {
    auto& [processor, _] = *iter;
    processor->Update(elapsed);

    if (size != processorCounts.size()) {
      Logger::Logf(LogLevel::critical, "Network processor list mutated in the middle of a loop!");
      break;
    }
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

  auto processorCountIter = processorCounts.find(processor.get());

  if (processorCountIter == processorCounts.end()) {
    processor->SetSocket(client);
    processorCounts[processor.get()] = 1;
  } else {
    processorCountIter->second += 1;
  }

  processor->OnListen(sender);
}

void NetManager::DropHandlers(const Poco::Net::SocketAddress& sender)
{
  for(auto& processor : handlers[sender]) {
    auto& count = processorCounts[processor.get()];
    processor->OnDrop(sender);

    count -= 1;

    if(count == 0) {
      // erase processor so it no longer gets updated
      processorCounts.erase(processor.get());
    }
  }

  handlers.erase(sender);
}

void NetManager::DropProcessor(const std::shared_ptr<IPacketProcessor>& processor)
{
  DropProcessor(processor.get());
}

void NetManager::DropProcessor(IPacketProcessor* processor)
{
  auto countIter = processorCounts.find(processor);

  if (countIter == processorCounts.end() || countIter->second == 0u) {
    // processor was never added
    return;
  }

  std::vector<Poco::Net::SocketAddress> handlersPendingRemoval;
  auto& count = processorCounts[processor];

  for (auto handlerIter = handlers.begin(); handlerIter != handlers.end();) {
    auto& [sender, processors] = *handlerIter;

    auto iter = std::find_if(processors.begin(), processors.end(), [processor](auto& p) { return p.get() == processor; });

    if (iter != processors.end()) {
      (*iter)->OnDrop(sender);
      processors.erase(iter);
      count -= 1;
    }

    if (processors.empty()) {
      handlerIter = handlers.erase(handlerIter);
      continue;
    }

    handlerIter++;
  }

  if (count == 0) {
    // erase processor so it no longer gets updated
    processorCounts.erase(processor);
  }

  for (auto& handler : handlersPendingRemoval) {
    handlers.erase(handler);
  }
}

void NetManager::SetMaxPayloadSize(uint16_t bytes)
{
  maxPayloadSize = bytes;
}

const uint16_t NetManager::GetMaxPayloadSize() const
{
  return maxPayloadSize;
}

const bool NetManager::BindPort(unsigned int port)
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

const std::string NetManager::GetPublicIP()
{
  std::string url = "checkip.amazonaws.com"; // send back public IP in plain text

  try {
    HTTPClientSession session(url);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    session.setTimeout(Poco::Timespan(10, 0));
    session.sendRequest(request);
    std::istream& rs = session.receiveResponse(response);

    if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      std::string temp = std::string(std::istreambuf_iterator<char>(rs), {});
      temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
      return temp;
    }
  }
  catch (std::exception& e) {
    Logger::Logf(LogLevel::critical, "PVP Network Exception while obtaining IP: %s", e.what());
  }

  // failed 
  return "";
}