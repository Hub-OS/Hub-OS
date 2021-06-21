#include "bnWebClientMananger.h"
#include "bnResourceHandle.h"
#include "bnURLParser.h"
#include "bnLogger.h"
#include "bnElements.h"
#include "bnCardUUIDs.h"
#include "bnTextureResourceManager.h"
#include <cstring>
#include <mutex>
#include <SFML/Network/Http.hpp>

void WebClientManager::PingThreadHandler()
{
  do {
    std::unique_lock<std::mutex> lock(clientMutex);

    if (!client) {
       isConnected = false;
    }
    else {
      isConnected = client->IsOK();
    }

    lock.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(GetPingInterval()));
  } while (!shutdownSignal);
}

void WebClientManager::QueuedTasksThreadHandler()
{
  std::unique_lock<std::mutex> lock(clientMutex);

  do {
    // wait until we have data or need to shut down
    taskQueueWakeup.wait(lock, [this] {
      return !taskQueue.empty() || shutdownSignal;
    });

    //after wait, we own the lock
    if (taskQueue.size())
    {
      isWorking = true;
      auto op = std::move(taskQueue.front());
      taskQueue.pop();

      lock.unlock();
      op();

      char* error;
      while (client && client->GetNextError(&error)) {
          Logger::Logf("Web Client encountered an error: %s", error);
      }

      lock.lock();
      isWorking = false;
    }
  } while (!shutdownSignal);
}

void WebClientManager::InitDownloadImageHandler()
{
  if (!client) return;

  auto callback = [](const char* url, WebAccounts::byte*& image, size_t& len) -> void {
    size_t size = 0;
    URL urlParser(url);

    sf::Http Http;
    sf::Http::Request request;
    unsigned short port = (unsigned short)strtoul(urlParser.GetPort().c_str(), nullptr, 0);

    Http.setHost(urlParser.GetHost(), port);
    request.setMethod(sf::Http::Request::Get);
    request.setUri(urlParser.GetPath()+urlParser.GetQuery());

    if (urlParser.GetHost().empty() || urlParser.GetPath().empty()) {
        image = 0;
        return;
    }

    sf::Http::Response Page = Http.sendRequest(request);

    std::string data = Page.getBody();
    len = data.size();

    if (data.empty()) {
        image = 0;
        return;
    }

    if (len > 0) {
        image = new WebAccounts::byte[len+1];
        data.copy((char*)image, len);
        image[len] = 0;
    }
  };

  std::scoped_lock<std::mutex> lock(this->clientMutex);
  client->SetDownloadImageHandler(callback);
}

void WebClientManager::CacheTextureData(const WebAccounts::AccountState& account)
{
  // The client should be valid and non-null ptr
  if (client) {
    std::shared_ptr<sf::Texture> comboIconTexture = std::make_shared<sf::Texture>();

    if (comboIconTexture->loadFromMemory(client->GetServerSettings().comboIconData, client->GetServerSettings().comboIconDataLen)) {
      for (auto&& combo : account.cardCombos) {
        iconTextureCache.insert(std::make_pair(combo.first, comboIconTexture));
      }
    }
  }

  for (auto&& card : account.cards) {
    auto&& cardModelIter = account.cardProperties.find(card.second->modelId);

    if (cardModelIter == account.cardProperties.end()) continue;

    const WebAccounts::byte* imageData = cardModelIter->second->imageData;
    const size_t imageDataLen = cardModelIter->second->imageDataLen;
    const WebAccounts::byte* iconData = cardModelIter->second->iconData;
    const size_t iconDataLen = cardModelIter->second->iconDataLen;

    std::shared_ptr<sf::Texture> textureObject = std::make_shared<sf::Texture>();

    bool imageSucceeded = (imageDataLen > 0);

    if (imageDataLen) {
      if (textureObject->loadFromMemory(imageData, imageDataLen)) {
          cardTextureCache.insert(std::make_pair(card.first,textureObject));
      }
      else {
        imageSucceeded = false;
      }
    }

    if (!imageSucceeded) {
      //Logger::Logf("Creating image data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
      textureObject = LOAD_TEXTURE(CHIP_MISSINGDATA);
      cardTextureCache.insert(std::make_pair(card.first,textureObject));
    }

    textureObject = std::make_shared<sf::Texture>(); // point to new texture

    imageSucceeded = (iconDataLen > 0);

    if (iconDataLen) {
      if (textureObject->loadFromMemory(iconData, iconDataLen)) {
          iconTextureCache.insert(std::make_pair(card.first,textureObject));
      }
      else {
        imageSucceeded = false;
      }
    }

    if (!imageSucceeded) {
      //Logger::Logf("Creating icon data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
      textureObject = LOAD_TEXTURE(CHIP_ICON_MISSINGDATA);
      iconTextureCache.insert(std::make_pair(card.first,textureObject));
    }
  }
}

void WebClientManager::SaveSession(const std::string& outpath)
{
  std::ofstream out(outpath, std::ios::binary);
  if (out.is_open()) {
    // Save version number
    out.write(version, version_len);

    // last API fetch
    out.write((char*)&account.lastFetchTimestamp, sizeof(long long));

    // username
    size_t username_len = this->username.size();
    out.write((char*)&username_len, sizeof(size_t));
    out.write((char*)this->username.c_str(), this->username.size());

    // Key-Values
    size_t hash_len = keys.size();
    out.write((char*)&hash_len, sizeof(size_t));
    for (auto&& pair : keys) {
      size_t key_len = pair.first.size();
      out.write((char*)&key_len, sizeof(size_t));
      out.write(pair.first.c_str(), key_len);

      size_t value_len = pair.second.size();
      out.write((char*)&value_len, sizeof(size_t));
      out.write(pair.second.c_str(), value_len);
    }

    // Card Image Data from Texture Cache
    size_t textureCacheLen = this->cardTextureCache.size();

    out.write((char*)&textureCacheLen, sizeof(size_t));
    for (auto&& pair : this->cardTextureCache) {
      size_t id_len = pair.first.size();
      out.write((char*)&id_len, sizeof(size_t));
      out.write(pair.first.c_str(), pair.first.size());

      auto image = pair.second->copyToImage();
      size_t image_len = static_cast<size_t>(image.getSize().x * image.getSize().y * 4u);
      out.write((char*)&image_len, sizeof(size_t));
      out.write((char*)image.getPixelsPtr(), image_len);
    }

    // Icon Image Data from Texture Cache
    size_t iconCacheLen = this->iconTextureCache.size();
    out.write((char*)&iconCacheLen, sizeof(size_t));

    for (auto&& pair : this->iconTextureCache) {
      size_t id_len = pair.first.size();
      out.write((char*)&id_len, sizeof(size_t));
      out.write(pair.first.c_str(), pair.first.size());

      auto image = pair.second->copyToImage();
      size_t image_len = static_cast<size_t>(image.getSize().x * image.getSize().y * 4u);
      out.write((char*)&image_len, sizeof(size_t));
      out.write((char*)image.getPixelsPtr(), image_len);
    }

    // Folders
    size_t numOfFolders = this->account.folders.size();
    out.write((char*)&numOfFolders, sizeof(size_t));

    for (auto&& pair : this->account.folders) {
      std::string name = pair.first;
      size_t name_len = name.size();
      out.write((char*)&name_len, sizeof(size_t));
      out.write(name.c_str(), name_len);

      size_t chip_count = pair.second->cards.size();
      out.write((char*)&chip_count, sizeof(size_t));

      for (auto&& card : pair.second->cards) {
        size_t card_id_len = card.size();
        out.write((char*)&card_id_len, sizeof(size_t));
        out.write(card.c_str(), card_id_len);
      }
    }

    // Card properties
    size_t card_props_list_len = this->account.cardProperties.size();
    out.write((char*)&card_props_list_len, sizeof(size_t));

    for (auto&& pair : this->account.cardProperties) {
      std::string id = pair.first;
      auto props = pair.second;
      size_t id_len = id.size();
      out.write((char*)&id_len, sizeof(size_t));
      out.write(id.c_str(), id_len);

      std::string name = props->name;
      size_t name_len = name.size();
      out.write((char*)&name_len, sizeof(size_t));
      out.write(name.c_str(), name_len);

      std::string action = props->action;
      size_t action_len = action.size();
      out.write((char*)&action_len, sizeof(size_t));
      out.write(action.c_str(), action_len);

      std::string element = props->element;
      size_t element_len = element.size();
      out.write((char*)&element_len, sizeof(size_t));
      out.write(element.c_str(), element_len);

      std::string secondary = props->secondaryElement;
      size_t secondary_len = secondary.size();
      out.write((char*)&secondary_len, sizeof(size_t));
      out.write(secondary.c_str(), secondary_len);

      out.write((char*)&props->classType, sizeof(decltype(props->classType)));
      out.write((char*)&props->damage, sizeof(decltype(props->damage)));
      out.write((char*)&props->timeFreeze, sizeof(decltype(props->timeFreeze)));
      out.write((char*)&props->limit, sizeof(decltype(props->limit)));

      // codes count
      auto codes = props->codes;
      size_t codes_len = codes.size();
      out.write((char*)&codes_len, sizeof(size_t));

      for (auto&& code : codes) {
        out.write((char*)&code, 1);
      }

      // meta classes count
      auto metaClasses = props->metaClasses;
      size_t meta_len = metaClasses.size();
      out.write((char*)&meta_len, sizeof(size_t));

      for (auto&& meta : metaClasses) {
        size_t meta_i_len = meta.size();
        out.write((char*)&meta_i_len, sizeof(size_t));
        out.write(meta.c_str(), meta.size());
      }

      std::string description = props->description;
      size_t desc_len = description.size();

      out.write((char*)&desc_len, sizeof(size_t));
      out.write(description.c_str(), description.size());

      std::string verboseDesc = props->verboseDescription;
      size_t verbose_len = verboseDesc.size();

      out.write((char*)&verbose_len, sizeof(size_t));
      out.write(verboseDesc.c_str(), verboseDesc.size());
    }

    // cards
    size_t cards_list_len = this->account.cards.size();
    out.write((char*)&cards_list_len, sizeof(size_t));

    for (auto&& pair : this->account.cards) {
      auto card = pair.second;
      char code = card->code;

      std::string id = card->id;
      size_t id_len = id.size();

      std::string modelId = card->modelId;
      size_t modelId_len = modelId.size();

      out.write((char*)&code, 1);

      out.write((char*)&id_len, sizeof(size_t));
      out.write((char*)id.c_str(), id_len);

      out.write((char*)&modelId_len, sizeof(size_t));
      out.write((char*)modelId.c_str(), modelId_len);
    }

    // combos
    size_t combos_list_len = this->account.cardCombos.size();
    out.write((char*)&combos_list_len, sizeof(size_t));

    for (auto&& pair : this->account.cardCombos) {
      auto combo = pair.second;

      std::string id = combo->id;
      size_t id_len = id.size();
      out.write((char*)&id_len, sizeof(size_t));
      out.write(id.c_str(), id_len);

      std::string name = combo->name;
      size_t name_len = name.size();
      out.write((char*)&name_len, sizeof(size_t));
      out.write(name.c_str(), name_len);

      std::string action = combo->action;
      size_t action_len = action.size();
      out.write((char*)&action_len, sizeof(size_t));
      out.write(action.c_str(), action_len);

      std::string element = combo->element;
      size_t element_len = element.size();
      out.write((char*)&element_len, sizeof(size_t));
      out.write(element.c_str(), element_len);

      std::string secondary = combo->secondaryElement;
      size_t secondary_len = secondary.size();
      out.write((char*)&secondary_len, sizeof(size_t));
      out.write(secondary.c_str(), secondary_len);

      size_t cards_list_len = combo->cards.size();
      out.write((char*)&cards_list_len, sizeof(size_t));
      for (auto&& card : combo->cards) {
        size_t card_id_len = card.size();
        out.write((char*)&card_id_len, sizeof(size_t));
        out.write((char*)card.c_str(), card.size());
      }

      size_t meta_list_len = combo->metaClasses.size();
      out.write((char*)&meta_list_len, sizeof(size_t));
      for (auto&& meta : combo->metaClasses) {
        size_t tag_len = meta.size();
        out.write((char*)&tag_len, sizeof(size_t));
        out.write((char*)meta.c_str(), meta.size());
      }

      int damage = combo->damage;
      out.write((char*)&damage, sizeof(int));

      bool timeFreeze = combo->timeFreeze;
      out.write((char*)&timeFreeze, sizeof(bool));
    }

    Logger::Logf("Session was written to %s!", outpath.c_str());
  }
  else {
    Logger::Logf("Saving session to %s failed", outpath.c_str());
  }
}

void WebClientManager::SetKey(const std::string& key, const std::string& value)
{
  auto iter = keys.find(key);

  if (iter != keys.end()) {
    keys[key] = value;
  }
  else {
    keys.insert(std::make_pair(key, value));
  }
}

const std::string WebClientManager::GetValue(const std::string& key)
{
  std::string res;

  auto iter = keys.find(key);
  if (iter != keys.end()) {
    res = iter->second;
  }

  return res;
}

const bool WebClientManager::LoadSession(const std::string& inpath, WebAccounts::AccountState* accountPtr)
{
  WebAccounts::AccountState& account = *accountPtr;

  std::ifstream in(inpath, std::ios::binary);
  if (in.is_open()) {
    // Re-use this local buffer for the entire load method
    char buffer[1024] = { 0 };

    // Read version 
    in.read(buffer, version_len);
    std::string versionStr{ buffer, version_len };
    std::string selfVersionStr{ version, version_len };

    // If the version string does not match, abort
    if(versionStr != selfVersionStr) {
      return false;
    }

    // last API fetch
    in.read((char*)&account.lastFetchTimestamp, sizeof(long long));

    // username
    size_t username_len = 0;
    in.read((char*)&username_len, sizeof(size_t));
    in.read(buffer, username_len);
    this->username = std::string(buffer, username_len);

    // Key-Values
    size_t hash_len = 0;
    in.read((char*)&hash_len, sizeof(size_t));

    while (hash_len-- > 0) {
      size_t key_len = 0;
      in.read((char*)&key_len, sizeof(size_t));
      in.read(buffer, key_len);
      std::string key{ buffer, key_len };

      size_t value_len = 0;
      in.read((char*)&value_len, sizeof(size_t));
      in.read(buffer, value_len);
      std::string value{ buffer, value_len };

      keys.insert(std::make_pair(key, value));
    }

    // Card Image Data from Texture Cache
    size_t textureCacheLen = 0;
    in.read((char*)&textureCacheLen, sizeof(size_t));

    while(textureCacheLen-- > 0) {
      size_t id_len = 0;
      in.read((char*)&id_len, sizeof(size_t));
      in.read(buffer, id_len);
      std::string id = std::string(buffer, id_len);

      size_t image_len = 0;
      in.read((char*)&image_len, sizeof(size_t));

      sf::Uint8* imageData = new  sf::Uint8[image_len]{ 0 };
      in.read((char*)imageData, image_len);

      sf::Image image;
      image.create(56, 48, imageData);
      auto textureObj = std::make_shared<sf::Texture>();
      textureObj->loadFromImage(image);

      this->cardTextureCache.insert(std::make_pair(id, textureObj));

      delete[] imageData;
    }

    // Icon Image Data from Texture Cache
    size_t iconCacheLen = 0;
    in.read((char*)&iconCacheLen, sizeof(size_t));

    while(iconCacheLen-- > 0) {
      size_t id_len = 0;
      in.read((char*)&id_len, sizeof(size_t));
      in.read(buffer, id_len);
      std::string id = std::string(buffer, id_len);

      size_t image_len = 0;
      in.read((char*)&image_len, sizeof(size_t));

      sf::Uint8* imageData = new  sf::Uint8[image_len] {0};
      in.read((char*)imageData, image_len);

      auto textureObj = std::make_shared<sf::Texture>();

      sf::Image image;
      image.create(14, 14, imageData);
      textureObj->loadFromImage(image);
      this->iconTextureCache.insert(std::make_pair(id, textureObj));

      delete[] imageData;
    }

    // Folders
    size_t numOfFolders = 0;
    in.read((char*)&numOfFolders, sizeof(size_t));

    while(numOfFolders-- > 0) {
      size_t name_len = 0;
      in.read((char*)&name_len, sizeof(size_t));
      in.read(buffer, name_len);
      std::string name = std::string(buffer, name_len);

      size_t chip_count = 0;
      in.read((char*)&chip_count, sizeof(size_t));

      std::vector<std::string> chips;

      while(chip_count-- > 0) {
        size_t card_id_len = 0;
        in.read((char*)&card_id_len, sizeof(size_t));
        in.read(buffer, card_id_len);
        std::string card_id = std::string(buffer, card_id_len);

        chips.push_back(card_id);
      }

      auto folder = std::make_shared<WebAccounts::Folder>();
      folder->name = name;
      folder->cards = chips;

      account.folders.insert(std::make_pair(name, folder));
    }

    // Card properties
    size_t card_props_list_len = 0;
    in.read((char*)&card_props_list_len, sizeof(size_t));

    while(card_props_list_len-- > 0 ) {
      size_t id_len = 0;
      in.read((char*)&id_len, sizeof(size_t));
      in.read(buffer, id_len);
      std::string id = std::string(buffer, id_len);

      size_t name_len = 0;
      in.read((char*)&name_len, sizeof(size_t));
      in.read(buffer, name_len);
      std::string name = std::string(buffer, name_len);

      size_t action_len = 0;
      in.read((char*)&action_len, sizeof(size_t));
      in.read(buffer, action_len);
      std::string action = std::string(buffer, action_len);

      size_t element_len = 0;
      in.read((char*)&element_len, sizeof(size_t));
      in.read(buffer, element_len);
      std::string element = std::string(buffer, element_len);

      size_t secondary_element_len = 0;
      in.read((char*)&secondary_element_len, sizeof(size_t));
      in.read(buffer, secondary_element_len);
      std::string secondary = std::string(buffer, secondary_element_len);

      auto props = std::make_shared<WebAccounts::CardProperties>();

      in.read((char*)&props->classType, sizeof(decltype(props->classType)));
      in.read((char*)&props->damage, sizeof(decltype(props->damage)));
      in.read((char*)&props->timeFreeze, sizeof(decltype(props->timeFreeze)));
      in.read((char*)&props->limit, sizeof(decltype(props->limit)));

      props->id = id;
      props->name = name;
      props->action = action;
      props->canBoost = props->damage > 0;
      props->element = element;
      props->secondaryElement = secondary;

      // codes count
      std::vector<char> codes;
      size_t codes_len = 0;
      in.read((char*)&codes_len, sizeof(size_t));

      while(codes_len-- > 0) {
        char code = 0;
        in.read((char*)&code, 1);
        codes.push_back(code);
      }

      props->codes = codes;

      // meta classes count
      std::vector<std::string> metaClasses;
      size_t meta_len = 0;
      in.read((char*)&meta_len, sizeof(size_t));

      while(meta_len-- > 0) {
        size_t meta_i_len = 0;
        in.read((char*)&meta_i_len, sizeof(size_t));
        in.read(buffer, meta_i_len);
        metaClasses.push_back(std::string(buffer, meta_i_len));
      }

      props->metaClasses = metaClasses;

      std::string description;
      size_t desc_len = 0;

      in.read((char*)&desc_len, sizeof(size_t));
      in.read(buffer, desc_len);
      description = std::string(buffer, desc_len);

      std::string verboseDesc;
      size_t verbose_len = 0;

      in.read((char*)&verbose_len, sizeof(size_t));
      in.read(buffer, verbose_len);
      verboseDesc = std::string(buffer, verbose_len);

      props->description = description;
      props->verboseDescription = verboseDesc;

      account.cardProperties.insert(std::make_pair(id, props));
    }

    // cards
    size_t cards_list_len = 0;
    in.read((char*)&cards_list_len, sizeof(size_t));

    while(cards_list_len-- > 0) {
      size_t id_len = 0;
      size_t modelId_len = 0;
      char code = 0;

      in.read((char*)&code, 1);

      in.read((char*)&id_len, sizeof(size_t));
      in.read(buffer, id_len);

      std::string id = std::string(buffer, id_len);

      in.read((char*)&modelId_len, sizeof(size_t));
      in.read(buffer, modelId_len);

      std::string modelId = std::string(buffer, modelId_len);

      auto card = std::make_shared<WebAccounts::Card>();
      card->id = id;
      card->code = code;
      card->modelId = modelId;

      account.cards.insert(std::make_pair(id, card));
    }

    // combos
    size_t combos_list_len = 0;
    in.read((char*)&combos_list_len, sizeof(size_t));

    while(combos_list_len-- > 0) {
      std::string id, name, action, element, secondary;

      size_t id_len = 0;
      in.read((char*)&id_len, sizeof(size_t));
      in.read(buffer, id_len);
      id = std::string(buffer, id_len);

      size_t name_len = 0;
      in.read((char*)&name_len, sizeof(size_t));
      in.read(buffer, name_len);
      name = std::string(buffer, name_len);

      size_t action_len = 0;
      in.read((char*)&action_len, sizeof(size_t));
      in.read(buffer, action_len);
      action = std::string(buffer, action_len);

      size_t element_len = 0;
      in.read((char*)&element_len, sizeof(size_t));
      in.read(buffer, element_len);
      element = std::string(buffer, element_len);

      size_t secondary_element_len = 0;
      in.read((char*)&secondary_element_len, sizeof(size_t));
      in.read(buffer, secondary_element_len);
      secondary = std::string(buffer, secondary_element_len);

      size_t cards_list_len = 0;
      in.read((char*)&cards_list_len, sizeof(size_t));

      std::vector<std::string> cards;

      while(cards_list_len-- > 0) {
        size_t card_id_len = 0;
        in.read((char*)&card_id_len, sizeof(size_t));
        in.read(buffer, card_id_len);
        cards.push_back(std::string(buffer, card_id_len));
      }

      size_t meta_list_len = 0;
      in.read((char*)&meta_list_len, sizeof(size_t));
      std::vector<std::string> metaClasses;

      while(meta_list_len-- > 0) {
        size_t tag_len = 0;
        in.read((char*)&tag_len, sizeof(size_t));
        in.read(buffer, tag_len);
        metaClasses.push_back(std::string(buffer, tag_len));
      }

      int damage = 0;
      in.read((char*)&damage, sizeof(int));

      bool timeFreeze = false;
      in.read((char*)&timeFreeze, sizeof(bool));

      auto combo = std::make_shared<WebAccounts::CardCombo>();
      combo->action = action;
      combo->canBoost = damage > 0;
      combo->cards = cards;
      combo->damage = damage;
      combo->element = element;
      combo->secondaryElement = secondary;
      combo->id = id;
      combo->metaClasses = metaClasses;
      combo->name = name;
      combo->timeFreeze = timeFreeze;

      account.cardCombos.insert(std::make_pair(id, combo));
    }

    Logger::Logf("Session was read from %s!", inpath.c_str());
    return true;
  }
  else {
    Logger::Logf("Reading session from %s failed", inpath.c_str());
  }

  return false;
}

void WebClientManager::UseCachedAccount(const WebAccounts::AccountState& account)
{
  this->account = account;
}

WebClientManager::WebClientManager() {
  shutdownSignal = false;
  isConnected = false;
  isWorking = false;
  client = nullptr;

  PingInterval(2000);

  pingThread = std::thread(&WebClientManager::PingThreadHandler, this);
  pingThread.detach();

  tasksThread = std::thread(&WebClientManager::QueuedTasksThreadHandler, this);
  tasksThread.detach();

  version = new char[version_len] {0};
  std::strcpy(version, "ONB WEBCLIENT V2.0");
}

WebClientManager::~WebClientManager()
{
  if(shutdownSignal == false) {
    ShutdownAllTasks();
  }

  delete[] version;
}

WebClientManager& WebClientManager::GetInstance() {
  static WebClientManager instance;
  return instance;
}

void WebClientManager::PingInterval(long interval) {
  heartbeatInterval = interval;
}

const long WebClientManager::GetPingInterval() const {
  return heartbeatInterval;
}

void WebClientManager::ConnectToWebServer(const char * apiVersion, const char * domain, int port)
{
  client = std::make_unique<WebAccounts::WebClient>(apiVersion, domain, port);
  InitDownloadImageHandler();
}

const bool WebClientManager::IsConnectedToWebServer()
{
  return isConnected;
}

const bool WebClientManager::IsLoggedIn()
{
  return client ? client->IsLoggedIn() : false;
}

const bool WebClientManager::IsWorking()
{
  return isWorking;
}

std::future<bool> WebClientManager::SendLoginCommand(const char * username, const char * password)
{
  auto promise = std::make_shared<std::promise<bool>>();

  char username_buffer[256]{ 0 };
  char password_buffer[256]{ 0 };
  size_t username_len = strlen(username);
  size_t password_len = strlen(password);

  std::memcpy((void*)&username_buffer[0], username, username_len);
  std::memcpy((void*)&password_buffer[0], password, password_len);

  auto task = [promise, username_buffer, password_buffer, this]() {
    if (!client) {
      // No valid client? Set to false immediately
      promise->set_value(false);
      return;
    }

    bool result = client->Login(username_buffer, password_buffer);

    if (result) {
      WebClientManager::username = username_buffer;
    }

    promise->set_value(result);
  };

  std::scoped_lock<std::mutex> lock(this->clientMutex);

  taskQueue.emplace(task);

  taskQueueWakeup.notify_all();

  return promise->get_future();
}

std::future<bool> WebClientManager::SendLogoutCommand()
{
  auto promise = std::make_shared<std::promise<bool>>();

  auto task = [promise, this]() {
    if (!client) {
      // No valid client? Set to false immediately
      promise->set_value(false);
      return;
    }

    client->LogoutAndReset();
    account = WebAccounts::AccountState(); // should effectively reset it

    // We should be logged out
    promise->set_value(!client->IsLoggedIn());
  };

  std::scoped_lock<std::mutex> lock(this->clientMutex);

  taskQueue.emplace(task);

  taskQueueWakeup.notify_all();

  return promise->get_future();
}

std::future<WebAccounts::AccountState> WebClientManager::SendFetchAccountCommand()
{
  auto promise = std::make_shared<std::promise<WebAccounts::AccountState>>();

  auto task = [promise, this]() {
    if (!client) {
      // No valid client? Don't send invalid data. Throw.
      promise->set_exception(std::make_exception_ptr(std::runtime_error("Could not get account data. Client object is invalid. Are you logged in?")));
      return;
    }

    client->FetchAccount(this->since);

    // Download these cards too:
    for (auto&& uuid : BuiltInCards::AsList) {
      Logger::Logf("Could fetch card %s? %s", uuid.data(), (client->FetchCard(uuid)? "yes": "no"));
    }

    account = client->GetLocalAccount();

    promise->set_value(account);

    this->since = CurrentTime::AsMilli();
  };

  std::scoped_lock<std::mutex> lock(this->clientMutex);

  taskQueue.emplace(task);

  taskQueueWakeup.notify_all();

  return promise->get_future();
}

std::future<WebClientManager::CardListCommandResult> WebClientManager::SendFetchCardListCommand(const std::vector<std::string>& cardList)
{
  auto promise = std::make_shared<std::promise<WebClientManager::CardListCommandResult>>();

  auto task = [promise, this, cardList]() {
    if (!client) {
      // No valid client? Don't send invalid data. Throw.
      promise->set_exception(std::make_exception_ptr(std::runtime_error("Could not get card data. Client object is invalid. Are you logged in?")));
      return;
    }

    WebClientManager::CardListCommandResult result;
    result.success = true; // assume we succeed

    for (const auto& uuid : cardList) {
      if (!client->FetchCard(uuid)) {
        Logger::Logf("Could not fetch card %s", uuid.data());
        result.failed.push_back(uuid);
        result.success = false;
      }
    }

    promise->set_value(result);
  };

  std::scoped_lock<std::mutex> lock(this->clientMutex);

  taskQueue.emplace(task);

  taskQueueWakeup.notify_all();

  return promise->get_future();
}

std::shared_ptr<sf::Texture> WebClientManager::GetIconForCard(const std::string & uuid)
{
  auto value = iconTextureCache[uuid];

  if (value == nullptr) {
    value = iconTextureCache[uuid] = LOAD_TEXTURE(CHIP_ICON_MISSINGDATA);
  }

  return value;
}

std::shared_ptr<sf::Texture> WebClientManager::GetImageForCard(const std::string & uuid)
{
  auto value = cardTextureCache[uuid];

  if (value == nullptr) {
    value = cardTextureCache[uuid] = LOAD_TEXTURE(CHIP_MISSINGDATA);
  }

  return value;
}

const Battle::Card WebClientManager::MakeBattleCardFromWebCardData(const WebAccounts::Card & card)
{
  std::string modelId = card.modelId;
  char code = card.code;

  if (modelId.empty()) {
    // try to find fill in the data
    auto cardDataIter = account.cards.find(card.id);

    if (cardDataIter != account.cards.end()) {
      modelId = cardDataIter->second->modelId;
      code = cardDataIter->second->code;
    }
  }
  auto cardModelIter = account.cardProperties.find(modelId);

  Battle::Card::Properties props;

  if (cardModelIter != account.cardProperties.end()) {
    auto cardModel = cardModelIter->second;

    props.uuid = card.id;
    props.code = card.code;
    props.shortname = cardModel->name;
    props.action = cardModel->action;
    props.damage = cardModel->damage;
    props.description = cardModel->description;
    props.verboseDescription = cardModel->verboseDescription;
    props.element = GetElementFromStr(cardModel->element);
    props.secondaryElement = GetElementFromStr(cardModel->secondaryElement);
    props.limit = cardModel->limit;
    props.timeFreeze = cardModel->timeFreeze;
    props.cardClass = static_cast<Battle::CardClass>(cardModel->classType);
    props.metaClasses = cardModel->metaClasses;
    props.canBoost = cardModel->canBoost;
  }

  return Battle::Card(props);
}

const Battle::Card WebClientManager::MakeBattleCardFromWebComboData(const WebAccounts::CardCombo& combo)
{
  std::string modelId = combo.id;

  auto comboIter = account.cardCombos.find(modelId);

  Battle::Card::Properties props;

  if (comboIter != account.cardCombos.end()) {
    auto combo = comboIter->second;

    props.uuid = combo->id; // prefix with a unique way of referencing this type of card
    props.code = '*'; // Doesn't matter really
    props.shortname = combo->name;
    props.action = combo->action;
    props.damage = combo->damage;
    props.description = "N/A"; // combos do not have descriptions
    props.verboseDescription = "N/A"; // combos do not have descriptions
    props.element = GetElementFromStr(combo->element);
    props.secondaryElement = GetElementFromStr(combo->secondaryElement);
    props.limit = 1; // Doesn't matter
    props.timeFreeze = combo->timeFreeze;
    props.cardClass = Battle::CardClass::standard; // combos do not have class types
    props.metaClasses = combo->metaClasses;
  }

  return Battle::Card(props);
}

const std::string & WebClientManager::GetUserName() const
{
  return username;
}

void WebClientManager::ShutdownAllTasks()
{
  shutdownSignal = true;

  std::unique_lock<std::mutex> lock(clientMutex);
  while (taskQueue.size()) {
    taskQueue.pop();
  }
  lock.unlock();

  taskQueueWakeup.notify_all();

  if (tasksThread.joinable()) {
    tasksThread.join();
  }

  if (pingThread.joinable()) {
    pingThread.join();
  }
}
