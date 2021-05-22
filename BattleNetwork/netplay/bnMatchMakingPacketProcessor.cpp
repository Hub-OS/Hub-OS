#include "bnMatchMakingPacketProcessor.h"
#include "bnMatchMakingScene.h"
#include "bnNetPlaySignals.h"

using namespace Poco;
using namespace Net;

MatchMaking::PacketProcessor::PacketProcessor(MatchMakingScene& scene) :
  scene(scene)
{
}

MatchMaking::PacketProcessor::~PacketProcessor()
{
}

void MatchMaking::PacketProcessor::OnPacket(char* buffer, int read, const Poco::Net::SocketAddress& sender) {
  if (read == 0)
    return;

  NetPlaySignals sig = *(NetPlaySignals*)buffer;
  size_t sigLen = sizeof(NetPlaySignals);
  Poco::Buffer<char> data{ 0 };
  data.append(buffer + sigLen, size_t(read) - sigLen);

  scene.ProcessPacketBody(sig, data);
}

void MatchMaking::PacketProcessor::Update(double elapsed) {
}

void MatchMaking::PacketProcessor::SetNewRemote(const std::string& socketAddressStr)
{
  validRemote = true;

  try {
    // TODO: does this throw? 
    remoteAddr = Poco::Net::SocketAddress(socketAddressStr);
  }
  catch (...) {
    validRemote = false;
  }
}

void MatchMaking::PacketProcessor::SendPacket(const Poco::Buffer<char>& data)
{
  client->sendTo(data.begin(), (int)data.size(), remoteAddr);
}

const Poco::Net::SocketAddress& MatchMaking::PacketProcessor::GetRemoteAddr()
{
  return remoteAddr;
}

const bool MatchMaking::PacketProcessor::RemoteAddrIsValid() const
{
  return validRemote;
}

const std::string MatchMaking::PacketProcessor::GetPublicIP()
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
