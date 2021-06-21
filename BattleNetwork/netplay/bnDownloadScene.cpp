#include "bnDownloadScene.h"
#include "../bnWebClientMananger.h"
#include <Segues/PixelateBlackWashFade.h>

DownloadScene::DownloadScene(swoosh::ActivityController& ac, const DownloadSceneProps& props) : 
  downloadSuccess(props.downloadSuccess),
  label(Font::Style::tiny),
  Scene(ac)
{
  downloadSuccess = false; 

  Logger::Logf("remote address is: %s", props.remoteAddress.toString().c_str());

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

  lastScreen = props.lastScreen;

  blur.setPower(40);
  blur.setTexture(&lastScreen);

  bg = sf::Sprite(lastScreen);
  bg.setColor(sf::Color(255, 255, 255, 200));

  setView(sf::Vector2u(480, 320));
}

DownloadScene::~DownloadScene()
{

}

void DownloadScene::sendCardList(const std::vector<std::string> uuids)
{
  size_t len = uuids.size();

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::card_list_download };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&len, sizeof(size_t));

  for (auto& id : uuids) {
    size_t len = id.size();
    buffer.append((char*)&len, sizeof(size_t));
    buffer.append(id.c_str(), len);
  }

  packetProcessor->UpdateHandshakeID(packetProcessor->SendPacket(Reliability::Reliable, buffer).second);
}

void DownloadScene::sendCustomPlayerData()
{
  // TODO:
}

void DownloadScene::sendPing()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ping };
  buffer.append((char*)&type, sizeof(NetPlaySignals));

  packetProcessor->SendPacket(Reliability::Unreliable, buffer);
}

void DownloadScene::recieveCardList(const Poco::Buffer<char>& buffer)
{
  // Tell web client to fetch and download these cards
  std::vector<std::string> cardList;
  size_t cardLen{};
  size_t read{};

  std::memcpy(&cardLen, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  retryCardList.clear();

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
    Logger::Logf("chip: %s", uuid.c_str());
  }

  retryCardList = cardList;
  
  if (cardList.empty()) {
    // Edge case
    // Nothing to download = done
    downloadSuccess = true; 
  }

  for (auto& uuid : cardList) {
    cardsToDownload[uuid] = "Downloading";
  }
}

void DownloadScene::recieveCustomPlayerData(const Poco::Buffer<char>& buffer)
{
  // TODO:
}

void DownloadScene::FetchCardList(const std::vector<std::string>& cardList)
{
  retryCardList = cardList;


  // Pressumptiously set all cards to "Complete"
  // if some are redownloading, only those will be
  // set to "Downloading" in the following lines...

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
  Logger::Logf("recieved header: %d", (int)header);

  switch (header) {
  case NetPlaySignals::card_list_download:
    if (!recievedRemoteCards) {
      this->recieveCardList(body);
      recievedRemoteCards = true;
    }
    break;
  case NetPlaySignals::custom_character_download:
    this->recieveCustomPlayerData(body);
    break;
  }
}

void DownloadScene::Complete()
{
  if (!downloadSuccess) {
    downloadSuccess = true;
    getController().pop();
  }
}

void DownloadScene::Abort(const std::vector<std::string>& failed)
{
  if (!aborting) {
    for (auto& uuid : failed) {
      cardsToDownload[uuid] = "Failed";
    }

    aborting = true;
  }
}

void DownloadScene::onUpdate(double elapsed)
{
  if (!packetProcessor->IsHandshakeAck() && !aborting) return;

  if (!webRequestSent && recievedRemoteCards) {
    FetchCardList(retryCardList);
    webRequestSent = true;
  }

  if (aborting) {
    abortingCountdown -= from_seconds(elapsed);
    if (abortingCountdown <= frames(0)) {
      // abort match
      using effect = swoosh::types::segue<PixelateBlackWashFade>;
      getController().pop<effect>();
    }

    return;
  }

  try {
    if (webRequestSent && fetchCardsResult.valid() && is_ready(fetchCardsResult)) {
      Logger::Logf("Trying to future::get() cards");
      auto res = fetchCardsResult.get();

      // if res.success == true the entire download is complete
      if (res.success) {
        Complete();
      }
      else if (tries++ < 3) {
        // Otherwise retry the items that failed a few more times...
        FetchCardList(res.failed);
      }
      else {
        // Quit trying
        Abort(res.failed);
      }
    }
  }
  catch (std::exception& err) {
    Logger::Logf("Error downloading card data: %s", err.what());

    // If we had cards to download, keep trying...
    if (retryCardList.size() && tries++ < 3) {
      FetchCardList(retryCardList);
    }
    else {
      // Otherwise omething may have gone terribly wrong
      Abort(retryCardList);
    }
  }
}

void DownloadScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  blur.apply(surface);

  float h = static_cast<float>(getController().getVirtualWindowSize().y);

  sf::Sprite icon;

  for (auto& [key, value] : cardsToDownload) {
    label.SetString(key + " - " + value);

    auto bounds = label.GetLocalBounds();
    float ydiff = bounds.height *label.getScale().y;

    if (value == "Downloading") {
      label.SetColor(sf::Color::White);
    } else if (value == "Complete") {
      label.SetColor(sf::Color::Green);
    }
    else {
      label.SetColor(sf::Color::Red);
    }

    if (auto iconTexture = WEBCLIENT.GetIconForCard(key)) {
      icon.setTexture(*iconTexture);
      float iconHeight = icon.getLocalBounds().height;
      icon.setOrigin(0, iconHeight);
    }

    icon.setPosition(sf::Vector2f(bounds.width+5.0f, bounds.height));
    label.setOrigin(sf::Vector2f(0, bounds.height));
    label.setPosition(0, h);

    h -= ydiff+5.0f;

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
