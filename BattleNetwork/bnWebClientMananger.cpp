#include "bnWebClientMananger.h"
#include "bnURLParser.h"
#include "bnLogger.h"
#include "bnElements.h"
#include "bnCardUUIDs.h"
#include "bnTextureResourceManager.h"
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
        //Wait until we have data
        taskQueueWakeup.wait(lock, [this] {
            return (taskQueue.size());
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
    // NOTE: By this step, the client should be valid and non-null ptr
    // assert(client);

    std::shared_ptr<sf::Texture> comboIconTexture = std::make_shared<sf::Texture>();
    
    if (comboIconTexture->loadFromMemory(client->GetServerSettings().comboIconData, client->GetServerSettings().comboIconDataLen)) {
      for (auto&& combo : account.cardCombos) {
        iconTextureCache.insert(std::make_pair(combo.first, comboIconTexture));
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
            Logger::Logf("Creating image data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
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
            Logger::Logf("Creating icon data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
            textureObject = LOAD_TEXTURE(CHIP_ICON_MISSINGDATA);
            iconTextureCache.insert(std::make_pair(card.first,textureObject));
        }
    }
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

    auto task = [promise, username, password, this]() {
        if (!client) {
            // No valid client? Set to false immediately
            promise->set_value(false);
            return;
        }

        bool result = client->Login(username, password);

        if (result) {
            WebClientManager::username = username;
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
    props.description = 'N/A'; // combos do not have descriptions
    props.verboseDescription = 'N/A'; // combos do not have descriptions
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
