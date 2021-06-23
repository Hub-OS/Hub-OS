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

      for (auto processor : handlers[sender]) {
        processor->OnPacket(buffer, read, sender);
      }
    }
    catch (Poco::Exception& e) {
      Logger::Logf("NetManager exception: %s", e.what());
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

  processor->OnListen(sender);
}

void NetManager::DropHandlers(const Poco::Net::SocketAddress& sender)
{
  for(auto& processor : handlers[sender]) {
    auto& count = processorCounts[processor];
    processor->OnDrop(sender);

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

  for (auto& [sender, processors] : handlers) {
    auto iter = std::find(processors.begin(), processors.end(), processor);

    if (iter != processors.end()) {
      (*iter)->OnDrop(sender);
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

const std::string NetManager::GetPublicIP()
{
  std::string url = "checkip.amazonaws.com"; // send back public IP in plain text

  try {
    HTTPClientSession session(url);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
    HTTPResponse response;

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
    Logger::Logf("PVP Network Exception while obtaining IP: %s", e.what());
  }

  // failed 
  return "";
}