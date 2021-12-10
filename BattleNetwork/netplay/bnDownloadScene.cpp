#include "bnDownloadScene.h"
#include "bnBufferReader.h"
#include "bnBufferWriter.h"
#include "../stx/string.h"
#include "../stx/zip_utils.h"
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
  playerCardPackageList = props.cardPackageHashes;

  std::sort(playerCardPackageList.begin(), playerCardPackageList.end());
  playerCardPackageList.erase(std::unique(playerCardPackageList.begin(), playerCardPackageList.end()), playerCardPackageList.end());

  downloadSuccess = false; 

  packetProcessor = props.packetProcessor;
  packetProcessor->SetKickCallback([this] {
    static bool once = false;

    if (!once) {
      Logger::Logf(LogLevel::info, "Kicked for silence!");
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
  BufferWriter writer;
  writer.Write(buffer, NetPlaySignals::download_handshake);
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

bool DownloadScene::AllTasksComplete()
{
  return cardPackageRequested && playerPackageRequested && contentToDownload.empty();
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

void DownloadScene::SendDownloadComplete(bool value)
{
  if (!downloadFlagSet) {
    downloadSuccess = value;
    downloadFlagSet = true;

    Poco::Buffer<char> buffer{ 0 };
    NetPlaySignals type{ NetPlaySignals::downloads_complete };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    buffer.append((char*)&downloadSuccess, sizeof(bool));

    packetProcessor->SendPacket(Reliability::Reliable, buffer);
  }
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
  case NetPlaySignals::download_handshake:
    Logger::Logf(LogLevel::info, "Remote is sending initial handshake");
    remoteHandshake = true;
    break;
  case NetPlaySignals::trade_card_package_list:
    Logger::Logf(LogLevel::info, "Remote is requesting to compare the card packages...");
    this->RecieveTradeCardPackageData(body);
    break;
  case NetPlaySignals::trade_player_package:
    Logger::Logf(LogLevel::info, "Remote is requesting to compare the player packages...");
    this->RecieveTradePlayerPackageData(body);
    break;
  case NetPlaySignals::player_package_request:
    Logger::Logf(LogLevel::info, "Remote is requesting to download the player package...");
    this->RecieveRequestPlayerPackageData(body);
    break;
  case NetPlaySignals::card_package_request:
    Logger::Logf(LogLevel::info, "Remote is requesting to download a card package...");
    this->RecieveRequestCardPackageData(body);
    break;
  case NetPlaySignals::card_package_download:
    Logger::Logf(LogLevel::info, "Downloading card package...");
    this->DownloadPackageData<CardPackageManager, ScriptedCard>(body, getController().CardPackageManager());
    break;
  case NetPlaySignals::player_package_download:
    Logger::Logf(LogLevel::info, "Downloading player package...");
    this->DownloadPlayerData(body);
    break;
  case NetPlaySignals::downloads_complete:
    this->RecieveDownloadComplete(body);
    break;
  }
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

  Logger::Logf(LogLevel::info, "Recieved remote's card package list size: %d", packageCardList.size());

  // move to the next state
  if (retryList.size()) {
    Logger::Logf(LogLevel::info, "Need to download %d card packages", packageCardList.size());
    RequestCardPackageList(retryList);
  }
  else {
    Logger::Logf(LogLevel::info, "Nothing to download.");
  }
  cardPackageRequested = true;
}

void DownloadScene::RecieveTradePlayerPackageData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  remotePlayerHash = hash;

  if (!getController().PlayerPackageManager().HasPackage(hash)) {
    contentToDownload[hash] = "Downloading";
    // This will download the player data and also set the remotePlayerHash when completed successfully
    RequestPlayerPackageData(hash);
  }

  playerPackageRequested = true;
}

void DownloadScene::RecieveRequestPlayerPackageData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (hash.size()) {
    Logger::Logf(LogLevel::info, "Recieved download request for player hash %s", hash.c_str());
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
    Logger::Logf(LogLevel::info, "Recieved download request for %s card package", hash.c_str());

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

  Logger::Logf(LogLevel::info, "Remote says download complete. Result: %s", result ? "Success" : "Fail");
}

void DownloadScene::DownloadPlayerData(const Poco::Buffer<char>& buffer)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (hash.empty()) return;
  RemoveFromDownloadList(hash);

  size_t file_len = reader.Read<size_t>(buffer);
  std::string path = "cache/" + stx::rand_alphanum(12) + ".zip";

  std::fstream file;
  file.open(path, std::ios::out | std::ios::binary);

  stx::result_t<std::string> result(std::nullptr_t{}, "Unset");

  if (file.is_open()) {
    while (file_len > 0) {
      char byte = reader.Read<char>(buffer);
      file << byte;
      file_len--;
    }

    file.close();

    remotePlayerHash = hash;
    result = getController().PlayerPackageManager().LoadPackageFromZip<ScriptedPlayer>(path);
  }
  
  if (result.is_error()) {
    Logger::Logf(LogLevel::critical, "Failed to download custom navi with hash %s: %s", hash.c_str(), result.error_cstr());

    // There was a problem creating the file
    SendDownloadComplete(false);
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

template<typename PackageManagerType, typename ScriptedDataType>
void DownloadScene::DownloadPackageData(const Poco::Buffer<char>& buffer, PackageManagerType& pm)
{
  BufferReader reader;
  std::string hash = reader.ReadTerminatedString(buffer);

  if (hash.empty()) return;
  RemoveFromDownloadList(hash);

  size_t file_len = reader.Read<size_t>(buffer);
  std::string path = "cache/" + stx::rand_alphanum(12) + ".zip";

  std::fstream file;
  file.open(path, std::ios::out | std::ios::binary);

  stx::result_t<std::string> result(std::nullptr_t{}, "Unset");

  if (file.is_open()) {
    while (file_len > 0) {
      char byte = reader.Read<char>(buffer);
      file << byte;
      file_len--;
    }

    file.close();

    result = pm.template LoadPackageFromZip<ScriptedDataType>(path);
  }

  if (result.is_error()) {
    Logger::Logf(LogLevel::critical, "Failed to download card package with hash %s: %s", hash.c_str(), result.error_cstr());

    // There was a problem creating the file
    SendDownloadComplete(false);
  }
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
    Logger::Logf(LogLevel::critical, "Could not serialize package: %s", result.error_cstr());

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
    Logger::Logf(LogLevel::critical, "Aborting");
    SendDownloadComplete(false);
    aborting = true;
  }
}

void DownloadScene::onUpdate(double elapsed)
{
  if (!(packetProcessor->IsHandshakeAck() && remoteHandshake) && !aborting) return;

  if (!hasTradedData) {
    hasTradedData = true;

    this->TradeCardPackageData(playerCardPackageList);
    this->TradePlayerPackageData(playerHash);
    return;
  }

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
  
  if (AllTasksComplete()) {
    SendDownloadComplete(true);

    if (downloadSuccess && remoteSuccess) {
      getController().pop();
    }
  }
}

void DownloadScene::onDraw(sf::RenderTexture& surface)
{
  auto& packageManager = getController().CardPackageManager();

  surface.draw(bg);
  blur.apply(surface);

  float w = static_cast<float>(getController().getVirtualWindowSize().x);
  float h = static_cast<float>(getController().getVirtualWindowSize().y);

  // 1. Draw the state status info
  if (AllTasksComplete()) {
    label.SetString("Complete, waiting...");

    auto bounds = label.GetLocalBounds();
    label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x, 0));
    label.setPosition(w, 0);
    label.SetColor(sf::Color::Green);
    surface.draw(label);
    return;
  }

  auto bounds = label.GetLocalBounds();
  label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x , 0));
  label.setPosition(w, 0);
  label.SetColor(sf::Color::White);
  surface.draw(label);

  sf::Sprite icon;

  for (auto& [key, value] : contentToDownload) {
    if (!packageManager.HasPackage(key)) continue;

    label.SetString(key + " - " + value);

    auto bounds = label.GetLocalBounds();
    float ydiff = bounds.height * label.getScale().y;

    if (auto iconTexture = packageManager.FindPackageByID(key).GetIconTexture()) {
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
  Logger::Logf(LogLevel::info, "onStart() sending handshake");
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