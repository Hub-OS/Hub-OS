#include "bnDownloadScene.h"
#include "../bnWebClientMananger.h"
#include <Swoosh/EmbedGLSL.h>
#include <Swoosh/Shaders.h>
#include <Segues/PixelateBlackWashFade.h>

DownloadScene::DownloadScene(swoosh::ActivityController& ac, const DownloadSceneProps& props) : 
  downloadSuccess(props.downloadSuccess),
  label(Font::Style::wide),
  Scene(ac)
{
  label.setScale(2.f, 2.f);

  downloadSuccess = false; 

  packetProcessor = std::make_shared<Netplay::PacketProcessor>(props.remoteAddress);
  packetProcessor->SetKickCallback([this] {
    this->Abort(retryCardList);
  });

  packetProcessor->EnableKickForSilence(true);

  packetProcessor->SetPacketBodyCallback([this](NetPlaySignals header, const Poco::Buffer<char>& body) {
    this->ProcessPacketBody(header, body);
  });

  Net().AddHandler(props.remoteAddress, packetProcessor);

  sendCardList(props.cardUUIDs);
  // TODO: sendCustomPlayerData();
  lastScreen = props.lastScreen;
  bg = sf::Sprite(lastScreen);
}

DownloadScene::~DownloadScene()
{

}

void DownloadScene::sendCardList(const std::vector<std::string> uuids)
{
  size_t len = uuids.size();

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::card_list_download };
  buffer.append((char*)&type, sizeof(NetPlaySignals::card_list_download));
  buffer.append((char*)&len, sizeof(size_t));

  for (auto& id : uuids) {
    size_t len = id.size();
    buffer.append((char*)&len, sizeof(size_t));
    buffer.append(id.c_str(), len);
  }

  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void DownloadScene::sendCustomPlayerData()
{
  // TODO:
}

void DownloadScene::recieveCardList(const Poco::Buffer<char>& buffer)
{
  // Tell web client to fetch and download these cards
  std::vector<std::string> cardList;
  size_t cardLen{};
  size_t read{};

  std::memcpy(&cardLen, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (cardLen > 0) {
    std::string uuid;
    size_t len{};
    std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);
    uuid.resize(len);
    std::memcpy(uuid.data(), buffer.begin() + read, len);
    read += len;
    cardList.push_back(uuid);
    cardLen--;

    cardsToDownload.insert(std::make_pair(uuid, ""));
  }

  if (cardList.empty()) {
    downloadSuccess = true; // TODO: wait for other opponent to finish downloading
  }
  else {
    FetchCardList(cardList);
  }
}

void DownloadScene::recieveCustomPlayerData(const Poco::Buffer<char>& buffer)
{
  // TODO:
}

void DownloadScene::FetchCardList(const std::vector<std::string>& cardList)
{
  retryCardList = cardList;

  for (auto& [_, value] : cardsToDownload) {
    value = "Complete";
  }

  fetchCardsResult = WEBCLIENT.SendFetchCardListCommand(cardList);

  for (auto& uuid : cardList) {
    cardsToDownload[uuid] = "Downloading";
  }
}

void DownloadScene::ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body)
{
  switch (header) {
  case NetPlaySignals::card_list_download:
    this->recieveCardList(body);
    break;
  case NetPlaySignals::custom_character_download:
    this->recieveCustomPlayerData(body);
    break;
  }
}

void DownloadScene::Abort(const std::vector<std::string>& failed)
{
  for (auto& uuid : failed) {
    cardsToDownload[uuid] = "Failed";
  }

  // abort match
  using effect = swoosh::types::segue<PixelateBlackWashFade>;
  getController().pop<effect>();
}

void DownloadScene::onUpdate(double elapsed)
{
  try {
    if (fetchCardsResult.valid()) {
      auto res = fetchCardsResult.get();

      if (res.success) {
        downloadSuccess = true;
        getController().pop();
      }
      else if (tries++ < 3) {
        FetchCardList(res.failed);
      }
      else {
        Abort(res.failed);
      }
    }
  }
  catch (std::future_error& err) {
    Logger::Logf("Error downloading card data: %s", err.what());

    if (tries++ < 3) {
      FetchCardList(retryCardList);
    }
    else {
      Abort(retryCardList);
    }
  }
}

void DownloadScene::onDraw(sf::RenderTexture& surface)
{
  // surface.draw(bg);

  float h = static_cast<float>(getController().getVirtualWindowSize().y);

  for (auto& [key, value] : cardsToDownload) {
    label.SetString(key + " - " + value);

    float ydiff = label.GetLocalBounds().height*label.getScale().y;

    if (value == "Downloading") {
      label.SetColor(sf::Color::White);
    } else if (value == "Complete") {
      label.SetColor(sf::Color::Green);
    }
    else {
      label.SetColor(sf::Color::Red);
    }

    label.setOrigin(sf::Vector2f(0, ydiff));
    label.setPosition(0, h);
    h -= ydiff;

    surface.draw(label);
  }
}

void DownloadScene::onLeave()
{
}

void DownloadScene::onExit()
{
}

void DownloadScene::onEnter()
{
}

void DownloadScene::onStart()
{
}

void DownloadScene::onResume()
{
}

void DownloadScene::onEnd()
{
  Net().DropProcessor(packetProcessor);
}
