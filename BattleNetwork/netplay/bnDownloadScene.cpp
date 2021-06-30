#include "bnDownloadScene.h"
#include "bnBufferReader.h"
#include "bnBufferWriter.h"
#include "../bnWebClientMananger.h"
#include <Segues/PixelateBlackWashFade.h>

DownloadScene::DownloadScene(swoosh::ActivityController& ac, const DownloadSceneProps& props) : 
  downloadSuccess(props.downloadSuccess),
  label(Font::Style::tiny),
  Scene(ac)
{
  ourCardList = props.cardUUIDs;
  downloadSuccess = false; 

  packetProcessor = props.packetProcessor;
  packetProcessor->SetKickCallback([this] {
    Logger::Logf("Kicked for silence!");
    this->Abort(retryCardList);
  });

  packetProcessor->EnableKickForSilence(true);

  packetProcessor->SetPacketBodyCallback([this](NetPlaySignals header, const Poco::Buffer<char>& body) {
    this->ProcessPacketBody(header, body);
  });

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

void DownloadScene::TradeCardList(const std::vector<std::string>& uuids)
{
  // Upload card list to remote. Track as the handshake.
  ourCardList = uuids;
  packetProcessor->UpdateHandshakeID(packetProcessor->SendPacket(Reliability::Reliable, SerializeUUIDs(NetPlaySignals::trade_card_list, uuids)).second);
}

void DownloadScene::RequestCardList(const std::vector<std::string>& uuids)
{
  packetProcessor->SendPacket(Reliability::Reliable, SerializeUUIDs(NetPlaySignals::card_list_request, uuids));
}

void DownloadScene::SendCustomPlayerData()
{
  // TODO:
}

void DownloadScene::SendDownloadComplete(bool success)
{
  downloadSuccess = success;

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::downloads_complete };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&downloadSuccess, sizeof(bool));

  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void DownloadScene::SendPing()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ping };
  buffer.append((char*)&type, sizeof(NetPlaySignals));

  packetProcessor->SendPacket(Reliability::Unreliable, buffer);
}

void DownloadScene::ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body)
{
  switch (header) {
  case NetPlaySignals::trade_card_list: 
    Logger::Logf("Recieved trade list download signal");
    if (currState == state::trade) {
      Logger::Logf("Processing trade list");
      this->RecieveTradeCardList(body);
    }
    break;
  case NetPlaySignals::card_list_request:
    this->RecieveRequestCardList(body);
    break;
  case NetPlaySignals::card_list_download:
    Logger::Logf("Recieved card list download signal");
    if (currState == state::download) {
      Logger::Logf("Downloading card list...");
      this->DownloadCardList(body);
    }
    break;
  case NetPlaySignals::downloads_complete:
    this->RecieveDownloadComplete(body);
    break;
  }
}

void DownloadScene::RecieveTradeCardList(const Poco::Buffer<char>& buffer)
{
  retryCardList.clear();
  auto remote = DeserializeUUIDs(buffer);

  for (auto& remoteUUID : remote) {
    if (auto ptr = WEBCLIENT.GetWebCard(remoteUUID); !ptr) {
      retryCardList.push_back(remoteUUID);
      cardsToDownload[remoteUUID] = "Downloading";
    }
  }

  Logger::Logf("Recieved remote's list size: %d", remote.size());

  // move to the next state
  if (retryCardList.empty()) {
    Logger::Logf("Nothing to download.");
    currState = state::complete;
    SendDownloadComplete(true);
  }
  else {
    Logger::Logf("Need to download %d cards", retryCardList.size());
    currState = state::download;
    RequestCardList(retryCardList);
  }
}

void DownloadScene::RecieveRequestCardList(const Poco::Buffer<char>& buffer)
{
  auto uuids = DeserializeUUIDs(buffer);

  Logger::Logf("Recieved download request for %d items", uuids.size());
  packetProcessor->SendPacket(Reliability::BigData, SerializeCards(uuids));
}

void DownloadScene::RecieveDownloadComplete(const Poco::Buffer<char>& buffer)
{
  bool result{};
  std::memcpy(&result, buffer.begin(), sizeof(bool));

  if (result) {
    remoteSuccess = true;
  }
  else {
    // downloadSuccess ref will tell matchmaking scene we failed
    Abort(retryCardList);
  }

  Logger::Logf("Remote says download complete. Result: %s", result ? "Success" : "Fail");
}

void DownloadScene::DownloadCardList(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  // Tell web client to fetch and download these cards
  std::vector<std::string> cardList;
  auto cardLen = reader.Read<uint16_t>(buffer);

  while (cardLen-- > 0) {
    // name
    std::string id = reader.ReadString<uint16_t>(buffer);

    // image data
    auto image_len = reader.Read<uint64_t>(buffer);

    sf::Uint8* imageData = new sf::Uint8[image_len]{ 0 };
    std::memcpy(imageData, buffer.begin() + reader.GetOffset(), image_len);
    reader.Skip(image_len);

    sf::Image image;
    image.create(56, 48, imageData);
    auto textureObj = std::make_shared<sf::Texture>();
    textureObj->loadFromImage(image);
    delete[] imageData;

    // icon data
    auto icon_len = reader.Read<uint64_t>(buffer);

    sf::Uint8* iconData = new  sf::Uint8[icon_len]{ 0 };
    std::memcpy(iconData, buffer.begin() + reader.GetOffset(), icon_len);
    reader.Skip(icon_len);

    sf::Image icon;
    icon.create(14, 14, iconData);
    auto iconObj = std::make_shared<sf::Texture>();
    iconObj->loadFromImage(icon);
    delete[] iconData;

    ////////////////////
    //   Card Model   //
    ////////////////////
    std::string model_id = reader.ReadString<uint16_t>(buffer);
    std::string name = reader.ReadString<uint16_t>(buffer);
    std::string action = reader.ReadString<uint16_t>(buffer);
    std::string element = reader.ReadString<uint16_t>(buffer);
    std::string secondary_element = reader.ReadString<uint16_t>(buffer);

    auto props = std::make_shared<WebAccounts::CardProperties>();

    props->classType = reader.Read<decltype(props->classType)>(buffer);
    props->damage = reader.Read<decltype(props->damage)>(buffer);
    props->timeFreeze = reader.Read<decltype(props->timeFreeze)>(buffer);
    props->limit = reader.Read<decltype(props->limit)>(buffer);

    props->id = model_id;
    props->name = name;
    props->action = action;
    props->canBoost = props->damage > 0;
    props->element = element;
    props->secondaryElement = secondary_element;

    // codes count
    std::vector<char> codes;
    auto codes_len = reader.Read<uint16_t>(buffer);

    while (codes_len-- > 0) {
      char code = reader.Read<char>(buffer);
      codes.push_back(code);
    }

    props->codes = codes;

    // meta classes count
    std::vector<std::string> metaClasses;
    auto meta_len = reader.Read<uint16_t>(buffer);

    while (meta_len-- > 0) {
      std::string meta = reader.ReadString<uint16_t>(buffer);
      metaClasses.push_back(meta);
    }

    props->metaClasses = metaClasses;

    props->description = reader.ReadString<uint16_t>(buffer);
    props->verboseDescription = reader.ReadString<uint16_t>(buffer);

    ////////////////////
    //  Card Data     //
    ////////////////////
    auto cardData = std::make_shared<WebAccounts::Card>();
    cardData->code = reader.Read<char>(buffer);
    cardData->id = reader.ReadString<uint16_t>(buffer);
    cardData->modelId = reader.ReadString<uint16_t>(buffer);

    // Upload
    WEBCLIENT.UploadCardData(id, iconObj, textureObj, cardData, props);
    cardsToDownload[id] = "Complete";
  }

  SendDownloadComplete(true);
  // move to the next state
  currState = state::complete;
}

std::vector<std::string> DownloadScene::DeserializeUUIDs(const Poco::Buffer<char>& buffer)
{
  size_t len{};
  size_t read{};
  std::vector<std::string> uuids;

  // list length
  std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (len > 0) {
    size_t id_len{};
    std::memcpy(&id_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    uuids.emplace_back(buffer.begin() + read, id_len);
    read += id_len;

    len--;
  }

  return uuids;
}

Poco::Buffer<char> DownloadScene::SerializeUUIDs(NetPlaySignals header, const std::vector<std::string>& uuids)
{
  Poco::Buffer<char> data{ 0 };

  // header
  data.append((char*)&header, sizeof(NetPlaySignals));

  // list length
  size_t len = uuids.size();
  data.append((char*)&len, sizeof(size_t));

  for(auto & id : uuids) {
    // name
    size_t sz = id.length();
    data.append((char*)&sz, sizeof(size_t));
    data.append(id.c_str(), id.length());
  }

  return data;
}

Poco::Buffer<char> DownloadScene::SerializeCards(const std::vector<std::string>& cardList)
{
  BufferWriter writer;
  Poco::Buffer<char> data{ 0 };
  size_t len = cardList.size();

  // header
  writer.Write(data, NetPlaySignals::card_list_download);

  // number of cards
  writer.Write(data, (uint16_t)cardList.size());

  for (auto& card : cardList) {
    // name
    writer.WriteString<uint16_t>(data, card);

    // image data
    auto texture = WEBCLIENT.GetImageForCard(card);
    auto image = texture->copyToImage();
    auto image_len = static_cast<uint64_t>(image.getSize().x * image.getSize().y * 4u);
    writer.Write(data, image_len);
    writer.WriteBytes(data, image.getPixelsPtr(), image_len);

    // icon data
    auto icon = WEBCLIENT.GetIconForCard(card);
    image = icon->copyToImage();
    image_len = static_cast<uint64_t>(image.getSize().x * image.getSize().y * 4u);
    writer.Write(data, image_len);
    writer.WriteBytes(data, image.getPixelsPtr(), image_len);

    ////////////////////
    // card meta data //
    ////////////////////

    auto cardData = WEBCLIENT.GetWebCard(card);
    auto model = WEBCLIENT.GetWebCardModel(cardData->modelId);

    writer.WriteString<uint16_t>(data, model->id);
    writer.WriteString<uint16_t>(data, model->name);
    writer.WriteString<uint16_t>(data, model->action);
    writer.WriteString<uint16_t>(data, model->element);
    writer.WriteString<uint16_t>(data, model->secondaryElement);

    writer.Write(data, model->classType);
    writer.Write(data, model->damage);
    writer.Write(data, model->timeFreeze);
    writer.Write(data, model->limit);

    // codes count
    std::vector<char> codes = model->codes;
    auto codes_len = (uint16_t)codes.size();
    writer.Write(data, codes_len);

    for (auto&& code : codes) {
      data.append(code);
    }

    // meta classes count
    auto metaClasses = model->metaClasses;
    auto meta_len = (uint16_t)metaClasses.size();
    writer.Write(data, meta_len);

    for (auto&& meta : metaClasses) {
      writer.WriteString<uint16_t>(data, meta);
    }

    writer.WriteString<uint16_t>(data, model->description);
    writer.WriteString<uint16_t>(data, model->verboseDescription);

    ///////////////
    // Card Data //
    ///////////////
    data.append(cardData->code);
    writer.WriteString<uint16_t>(data, cardData->id);
    writer.WriteString<uint16_t>(data, cardData->modelId);
  }

  return data;
}

void DownloadScene::Abort(const std::vector<std::string>& failed)
{
  Logger::Logf("Aborting");

  if (!aborting) {
    for (auto& uuid : failed) {
      cardsToDownload[uuid] = "Failed";
    }

    SendDownloadComplete(false);
    aborting = true;
  }
}

void DownloadScene::onUpdate(double elapsed)
{
  if (!packetProcessor->IsHandshakeAck() && !aborting) return;

  SendPing();

  if (aborting) {
    abortingCountdown -= from_seconds(elapsed);
    if (abortingCountdown <= frames(0)) {
      // abort match
      using effect = swoosh::types::segue<PixelateBlackWashFade>;
      getController().pop<effect>();
    }

    return;
  }
  else if (downloadSuccess && remoteSuccess) {
    getController().pop();
  }
}

void DownloadScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  blur.apply(surface);

  float w = static_cast<float>(getController().getVirtualWindowSize().x);
  float h = static_cast<float>(getController().getVirtualWindowSize().y);

  // 1. Draw the state status info
  switch (currState) {
  case state::trade:
    label.SetString("Connecting to other player...");
    break;
  case state::download:
    label.SetString("Downloading, please wait...");
    break;
  case state::complete:
    label.SetString("Complete, waiting...");
    break;
  }

  auto bounds = label.GetLocalBounds();
  label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x , 0));
  label.setPosition(w, 0);
  label.SetColor(sf::Color::White);
  surface.draw(label);

  if (currState < state::download) {
    label.SetString("Request download, please wait...");
    auto bounds = label.GetLocalBounds();
    label.SetColor(sf::Color::White);
    label.setOrigin(sf::Vector2f(0, bounds.height));
    label.setPosition(0, h);
    surface.draw(label);
  }
  else {
    sf::Sprite icon;

    if (cardsToDownload.empty()) {
      label.SetString("Nothing to download.");
      auto bounds = label.GetLocalBounds();
      label.SetColor(sf::Color::White);
      label.setOrigin(sf::Vector2f(0, bounds.height));
      label.setPosition(0, h);
      surface.draw(label);
    }

    for (auto& [key, value] : cardsToDownload) {
      label.SetString(key + " - " + value);

      auto bounds = label.GetLocalBounds();
      float ydiff = bounds.height * label.getScale().y;

      if (value == "Downloading") {
        label.SetColor(sf::Color::White);
      }
      else if (value == "Complete") {
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

      icon.setPosition(sf::Vector2f(bounds.width + 5.0f, bounds.height));
      label.setOrigin(sf::Vector2f(0, bounds.height));
      label.setPosition(0, h);

      h -= ydiff + 5.0f;

      surface.draw(label);
    }
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
  TradeCardList(ourCardList);
}

void DownloadScene::onResume()
{
}

void DownloadScene::onEnd()
{
}