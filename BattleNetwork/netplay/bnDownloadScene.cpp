#include "bnDownloadScene.h"
#include "bnBufferReader.h"
#include "bnBufferWriter.h"
#include "../stx/string.h"
#include "../stx/zip_utils.h"
#include "../bnWebClientMananger.h"
#include "../bnPlayer.h"
#include "../bindings/bnScriptedPlayer.h"
#include "../bnPackageManager.h"
#include "../bnPlayerPackageManager.h"
#include "../bnCardPackageManager.h"
#include "../bindings/bnScriptedCard.h"
#include <Segues/PixelateBlackWashFade.h>

constexpr std::string_view CACHE_FOLDER = "cache";

DownloadScene::DownloadScene(swoosh::ActivityController& ac, const DownloadSceneProps& props) : 
  downloadSuccess(props.downloadSuccess),
  lastScreen(props.lastScreen),
  playerHash(props.playerHash),
  remotePlayerHash(props.remotePlayerHash),
  label(Font::Style::tiny),
  Scene(ac)
{
  playerWebCardList = props.webCardsUUIDs;
  playerCardPackageList = props.cardPackageHashes;

  std::sort(playerWebCardList.begin(), playerWebCardList.end());
  playerWebCardList.erase(std::unique(playerWebCardList.begin(), playerWebCardList.end()), playerWebCardList.end());

  std::sort(playerCardPackageList.begin(), playerCardPackageList.end());
  playerCardPackageList.erase(std::unique(playerCardPackageList.begin(), playerCardPackageList.end()), playerCardPackageList.end());

  downloadSuccess = false; 

  packetProcessor = props.packetProcessor;
  packetProcessor->SetKickCallback([this] {
    static bool once = false;

    if (!once) {
      Logger::Logf("Kicked for silence!");
      once = true;
    }

    this->Abort();
  });

  packetProcessor->EnableKickForSilence(true);

  packetProcessor->SetPacketBodyCallback([this](NetPlaySignals header, const Poco::Buffer<char>& body) {
    this->ProcessPacketBody(header, body);
  });

  blur.setPower(40);
  blur.setTexture(&lastScreen);

  bg = sf::Sprite(lastScreen);
  bg.setColor(sf::Color(255, 255, 255, 200));

  setView(sf::Vector2u(480, 320));

  std::filesystem::create_directories(CACHE_FOLDER);
}

DownloadScene::~DownloadScene()
{

}

void DownloadScene::SendHandshakeAck()
{
  Poco::Buffer<char> buffer(0);
  std::string payload = "download_handshake";
  buffer.append(payload.c_str(), payload.size());
  auto id = packetProcessor->SendPacket(Reliability::Reliable, buffer).second;
  packetProcessor->UpdateHandshakeID(id);
}

void DownloadScene::RemoveFromDownloadList(const std::string& id)
{
  auto iter = contentToDownload.find(id);
  if (iter != contentToDownload.end()) {
    contentToDownload.erase(iter);
  }
}

bool DownloadScene::ProcessTaskQueue()
{
  if (currTask == DownloadScene::TaskTypes::wait && currTask == nextTask) return !AllTasksComplete();

  currTask = nextTask;

  switch (currTask) {
  case TaskTypes::trade_web_cards:
    this->TradeWebCardList(playerWebCardList);
    break;
  case TaskTypes::trade_card_packages:
    this->TradeCardPackageData(playerCardPackageList);
    break;
  case TaskTypes::trade_player_package:
    this->TradePlayerPackageData(playerHash);
    break;
  default:
    break;
  }
  
  return true;
}

bool DownloadScene::AllTasksComplete()
{
  return contentToDownload.empty();
}

void DownloadScene::TradeWebCardList(const std::vector<std::string>& uuids)
{
  // Upload card list to remote. Track as the handshake.
  packetProcessor->SendPacket(Reliability::Reliable, SerializeListOfStrings(NetPlaySignals::trade_web_card_list, uuids));
}

void DownloadScene::TradePlayerPackageData(const std::string& hash)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write<NetPlaySignals>(buffer, NetPlaySignals::trade_player_package);
  writer.WriteTerminatedString(buffer, hash);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void DownloadScene::TradeCardPackageData(const std::vector<std::string>& hashes)
{
  // Upload card list to remote. Track as the handshake.
  packetProcessor->SendPacket(Reliability::Reliable, SerializeListOfStrings(NetPlaySignals::trade_card_package_list, hashes));
}

void DownloadScene::RequestWebCardList(const std::vector<std::string>& uuids)
{
  packetProcessor->SendPacket(Reliability::Reliable, SerializeListOfStrings(NetPlaySignals::web_card_list_request, uuids));
}

void DownloadScene::RequestPlayerPackageData(const std::string& hash)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write<NetPlaySignals>(buffer, NetPlaySignals::player_package_request);
  writer.WriteTerminatedString(buffer, hash);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void DownloadScene::RequestCardPackageList(const std::vector<std::string>& hashes)
{
  for (auto& hash : hashes) {
    BufferWriter writer;
    Poco::Buffer<char> buffer{ 0 };
    writer.Write<NetPlaySignals>(buffer, NetPlaySignals::card_package_request);
    writer.WriteTerminatedString(buffer, hash);
    packetProcessor->SendPacket(Reliability::Reliable, buffer);
  }
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
  case NetPlaySignals::trade_web_card_list:
    Logger::Logf("Remote is requesting to compare the card list...");
    this->RecieveTradeWebCardList(body);
    break;
  case NetPlaySignals::trade_player_package:
    Logger::Logf("Remote is requesting to compare the player packages...");
    this->RecieveTradePlayerPackageData(body);
    break;
  case NetPlaySignals::trade_card_package_list:
    Logger::Logf("Remote is requesting to compare the card packages...");
    this->RecieveTradeCardPackageData(body);
    break;
  case NetPlaySignals::web_card_list_request:
    Logger::Logf("Remote is requesting to download the card list...");
    this->RecieveRequestWebCardList(body);
    break;
  case NetPlaySignals::player_package_request:
    Logger::Logf("Remote is requesting to download the player package...");
    this->RecieveRequestPlayerPackageData(body);
    break;
  case NetPlaySignals::card_package_request:
    Logger::Logf("Remote is requesting to download a card package...");
    this->RecieveRequestCardPackageData(body);
    break;
  case NetPlaySignals::web_card_list_download:
    Logger::Logf("Downloading card list...");
    this->DownloadCardList(body);
    break;
  case NetPlaySignals::player_package_download:
    Logger::Logf("Downloading player package...");
    this->DownloadPlayerData(body);
    break;
  case NetPlaySignals::card_package_download:
    Logger::Logf("Downloading card package...");
    this->DownloadPackageData<CardPackageManager, ScriptedCard>(body, getController().CardPackageManager());
    break;
  case NetPlaySignals::downloads_complete:
    this->RecieveDownloadComplete(body);
    break;
  }
}

void DownloadScene::RecieveTradeWebCardList(const Poco::Buffer<char>& buffer)
{
  auto cardList = DeserializeListOfStrings(buffer);
  std::vector<std::string> retryCardList;

  for (auto& remoteUUID : cardList) {
    if (auto ptr = WEBCLIENT.GetWebCard(remoteUUID); !ptr) {
      retryCardList.push_back(remoteUUID);
      contentToDownload[remoteUUID] = "Downloading";
    }
  }

  // move to the next state
  if (retryCardList.size()) {
    Logger::Logf("Need to download %d cards", retryCardList.size());
  }
  else {
    Logger::Logf("Nothing to download.");
  }

  nextTask = DownloadScene::TaskTypes::trade_card_packages;
  RequestWebCardList(retryCardList);
}

void DownloadScene::RecieveTradeCardPackageData(const Poco::Buffer<char>& buffer)
{
  auto packageCardList = DeserializeListOfStrings(buffer);

  std::vector<std::string> retryList;

  auto& packageManager = getController().CardPackageManager();
  for (auto& remoteHash : packageCardList) {
    if (!packageManager.HasPackage(remoteHash)) {
      retryList.push_back(remoteHash);
      contentToDownload[remoteHash] = "Downloading";
    }
  }

  Logger::Logf("Recieved remote's card package list size: %d", packageCardList.size());

  // move to the next state
  if (retryList.size()) {
    Logger::Logf("Need to download %d card packages", packageCardList.size());
  }
  else {
    Logger::Logf("Nothing to download.");
  }

  nextTask = DownloadScene::TaskTypes::trade_player_package;
  RequestCardPackageList(retryList);
}

void DownloadScene::RecieveTradePlayerPackageData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (getController().PlayerPackageManager().HasPackage(hash)) {
    remotePlayerHash = hash;
    hash = "";
  }
  else {
    contentToDownload[hash] = "Downloading";
  }

  nextTask = DownloadScene::TaskTypes::wait;
  // This will download the player data and also set the remotePlayerHash when completed successfully
  RequestPlayerPackageData(hash);
}

void DownloadScene::RecieveRequestWebCardList(const Poco::Buffer<char>& buffer)
{
  auto uuids = DeserializeListOfStrings(buffer);

  Logger::Logf("Recieved download request for %d items", uuids.size());
  packetProcessor->SendPacket(Reliability::BigData, SerializeCards(uuids));
}

void DownloadScene::RecieveRequestPlayerPackageData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (hash.size()) {
    Logger::Logf("Recieved download request for player hash %s", hash.c_str());
    packetProcessor->SendPacket(Reliability::BigData,
      SerializePackageData(
        hash,
        NetPlaySignals::player_package_download,
        getController().PlayerPackageManager()
      )
    );
  }
}

void DownloadScene::RecieveRequestCardPackageData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (!hash.empty()) {
    Logger::Logf("Recieved download request for %s card package", hash.c_str());

    packetProcessor->SendPacket(Reliability::BigData,
      SerializePackageData<CardPackageManager>(
        hash,
        NetPlaySignals::card_package_download,
        getController().CardPackageManager()
        )
    );
  }
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
    Abort();
  }

  Logger::Logf("Remote says download complete. Result: %s", result ? "Success" : "Fail");
}

void DownloadScene::DownloadPlayerData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);
  RemoveFromDownloadList(hash);

  if (hash.empty()) return;

  size_t file_len = reader.Read<size_t>(buffer);
  std::string path = "cache/" + stx::rand_alphanum(12) + ".zip";

  std::fstream file;
  file.open(path, std::ios::out | std::ios::binary);

  if (file.is_open()) {
    while (file_len > 0) {
      char byte = reader.Read<char>(buffer);
      file << byte;
      file_len--;
    }

    file.close();

    if (auto result = getController().PlayerPackageManager().LoadPackageFromZip<ScriptedPlayer>(path); result.value()) {
      Logger::Logf("Successesfully downloaded custom navi with hash %s", hash.c_str());
      remotePlayerHash = hash;
      return;
    }
  }
  
  Logger::Logf("Failed to download custom navi with hash %s", hash.c_str());

  // There was a problem creating the file
  SendDownloadComplete(false);
}

void DownloadScene::DownloadCardList(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  // Tell web client to fetch and download these cards
  std::vector<std::string> cardList;
  size_t cardLen = reader.Read<uint16_t>(buffer);

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
    
    RemoveFromDownloadList(id);
  }
}

std::vector<std::string> DownloadScene::DeserializeListOfStrings(const Poco::Buffer<char>& buffer)
{
  size_t len{};
  size_t read{};
  std::vector<std::string> list;

  // list length
  std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (len > 0) {
    size_t id_len{};
    std::memcpy(&id_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    list.emplace_back(buffer.begin() + read, id_len);
    read += id_len;

    len--;
  }

  return list;
}

Poco::Buffer<char> DownloadScene::SerializeListOfStrings(NetPlaySignals header, const std::vector<std::string>& list)
{
  Poco::Buffer<char> data{ 0 };

  // header
  data.append((char*)&header, sizeof(NetPlaySignals));

  // list length
  size_t len = list.size();
  data.append((char*)&len, sizeof(size_t));

  for(auto & id : list) {
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
  writer.Write(data, NetPlaySignals::web_card_list_download);

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
    auto& metaClasses = model->metaClasses;
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

template<typename PackageManagerType, typename ScriptedDataType>
void DownloadScene::DownloadPackageData(const Poco::Buffer<char>& buffer, PackageManagerType& pm)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  size_t file_len = reader.Read<size_t>(buffer);
  std::string path = "cache/" + stx::rand_alphanum(12) + ".zip";

  std::fstream file;
  file.open(path, std::ios::out | std::ios::binary);

  if (file.is_open()) {
    while (file_len > 0) {
      char byte = reader.Read<char>(buffer);
      file << byte;
      file_len--;
    }

    file.close();

    auto result = pm.template LoadPackageFromZip<ScriptedDataType>(path);

    RemoveFromDownloadList(hash);
  }

  Logger::Logf("Failed to download card package with hash %s", hash.c_str());

  // There was a problem creating the file
  SendDownloadComplete(false);
}

template<typename PackageManagerType>
Poco::Buffer<char> DownloadScene::SerializePackageData(const std::string& hash, NetPlaySignals header, PackageManagerType& packageManager)
{
  Poco::Buffer<char> buffer{ 0 };
  std::vector<char> fileBuffer;

  BufferWriter writer;
  size_t len = 0;
  
  auto result = packageManager.GetPackageFilePath(hash);
  if (result.is_error()) {
    Logger::Logf("Could not serialize package: %s", result.error_cstr());

    // Give the remote client a headsup abort
    SendDownloadComplete(false);
  }
  else {
    std::string path = result.value();
    
    if (auto result = stx::zip(path, path + ".zip"); result.value()) {
      path = path + ".zip";

      std::ifstream fs(path, std::ios::binary | std::ios::ate);
      std::ifstream::pos_type pos = fs.tellg();
      len = pos;
      fileBuffer.resize(len);
      fs.seekg(0, std::ios::beg);
      fs.read(&fileBuffer[0], pos);
    }
  }

  // header
  writer.Write(buffer, header);

  // hash name
  writer.WriteTerminatedString(buffer, hash);

  // file size
  writer.Write<size_t>(buffer, len);

  // file contents
  writer.WriteBytes<char>(buffer, fileBuffer.data(), fileBuffer.size());

  return buffer;
}

void DownloadScene::Abort()
{
  if (!aborting) {
    for (auto& [key, value] : contentToDownload) {
      value = "Failed";
    }
    Logger::Logf("Aborting");
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
  
  if (this->ProcessTaskQueue()) {
    return;
  }
  
  if (downloadSuccess && remoteSuccess) {
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
  label.SetString("Complete, waiting...");

  if (AllTasksComplete()) {
    auto bounds = label.GetLocalBounds();
    label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x, 0));
    label.setPosition(w, 0);
    label.SetColor(sf::Color::Green);
    surface.draw(label);
    return;
  }

  switch (currTask) {
  case TaskTypes::trade_web_cards:
    label.SetString("Fetching remote web cards...");
    break;
  case TaskTypes::trade_player_package:
    label.SetString("Fetching other player package...");
    break;
  case TaskTypes::trade_card_packages:
    label.SetString("Fetching remote card packages...");
    break;
  default:
    break;
  }

  auto bounds = label.GetLocalBounds();
  label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x , 0));
  label.setPosition(w, 0);
  label.SetColor(sf::Color::White);
  surface.draw(label);

  sf::Sprite icon;

  if (contentToDownload.empty()) {
    label.SetString("Nothing to download.");
    auto bounds = label.GetLocalBounds();
    label.SetColor(sf::Color::White);
    label.setOrigin(sf::Vector2f(0, bounds.height));
    label.setPosition(0, h);
    surface.draw(label);
  }

  for (auto& [key, value] : contentToDownload) {
    label.SetString(key + " - " + value);

    auto bounds = label.GetLocalBounds();
    float ydiff = bounds.height * label.getScale().y;

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
  Logger::Logf("onStart() trying to trade card data");
  SendHandshakeAck();
}

void DownloadScene::onResume()
{
}

void DownloadScene::onEnd()
{
  packetProcessor->SetPacketBodyCallback(nullptr);
  packetProcessor->SetKickCallback(nullptr);
}