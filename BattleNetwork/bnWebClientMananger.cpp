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
        std::unique_lock<std::mutex> lock(this->clientMutex);

        if (!this->client) {
            this->isConnected = false;
        }
        else {
            this->isConnected = this->client->IsOK();
        }      

        lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(this->GetPingInterval()));
    } while (!shutdownSignal);
}

void WebClientManager::QueuedTasksThreadHandler()
{
    std::unique_lock<std::mutex> lock(this->clientMutex);

    do {
        //Wait until we have data
        taskQueueWakeup.wait(lock, [this] {
            return (taskQueue.size());
        });

        //after wait, we own the lock
        if (taskQueue.size())
        {
            this->isWorking = true;
            auto op = std::move(taskQueue.front());
            taskQueue.pop();

            lock.unlock();
            op();
            lock.lock();
            this->isWorking = false;
        }
    } while (!shutdownSignal);
}

void WebClientManager::InitDownloadImageHandler()
{
    if (!this->client) return;

    auto callback = [](const char* url, WebAccounts::byte*& image, size_t& len) -> void {
        size_t size = 0;
        URL urlParser(url);

        sf::Http Http;
        sf::Http::Request request;

        Http.setHost(urlParser.GetHost());
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
            data.copy(image, len);
            image[len] = 0;
        }
    };

    std::scoped_lock<std::mutex>(this->clientMutex);
    this->client->SetDownloadImageHandler(callback);
}

void WebClientManager::CacheTextureData(const WebAccounts::AccountState& account)
{
    for (auto&& card : account.cards) {
        auto&& cardModelIter = account.cardModels.find(card.second->modelId);

        if (cardModelIter == account.cardModels.end()) continue;

        const WebAccounts::byte* imageData = cardModelIter->second->imageData;
        const size_t imageDataLen = cardModelIter->second->imageDataLen;
        const WebAccounts::byte* iconData = cardModelIter->second->iconData;
        const size_t iconDataLen = cardModelIter->second->iconDataLen;

        std::shared_ptr<sf::Texture> textureObject = std::make_shared<sf::Texture>();

        bool imageSucceeded = (imageDataLen > 0);

        if (imageDataLen) {
            if (textureObject->loadFromMemory(imageData, imageDataLen)) {
                this->cardTextureCache.insert(std::make_pair(card.first,textureObject));
                imageSucceeded = true;
            }
        }

        if (!imageSucceeded) {
            Logger::Logf("Creating image data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
            textureObject = LOAD_TEXTURE(CHIP_MISSINGDATA);
            this->cardTextureCache.insert(std::make_pair(card.first,textureObject));
        }
        
        textureObject.reset();
        imageSucceeded = (iconDataLen > 0);

        if (iconDataLen) {
            if (textureObject->loadFromMemory(iconData, iconDataLen)) {
                this->iconTextureCache.insert(std::make_pair(card.first,textureObject));
            }
        }

        if (!imageSucceeded) {
            Logger::Logf("Creating icon data for card (%s, %s) failed", cardModelIter->first.c_str(), cardModelIter->second->name.c_str());
            textureObject = LOAD_TEXTURE(CHIP_ICON_MISSINGDATA);
            this->iconTextureCache.insert(std::make_pair(card.first,textureObject));
        }
    }
}

WebClientManager::WebClientManager() {
    shutdownSignal = false;
    isConnected = false;
    isWorking = false;

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
    this->heartbeatInterval = interval;
}

const long WebClientManager::GetPingInterval() const {
    return this->heartbeatInterval;
}

void WebClientManager::ConnectToWebServer(const char * apiVersion, const char * domain, int port)
{
    this->client = std::make_unique<WebAccounts::WebClient>(apiVersion, domain, port);
    InitDownloadImageHandler();
}

const bool WebClientManager::IsConnectedToWebServer()
{
    return isConnected;
}

const bool WebClientManager::IsLoggedIn()
{
    return this->client ? this->client->IsLoggedIn() : false;
}

const bool WebClientManager::IsWorking()
{
    return this->isWorking;
}

std::future<bool> WebClientManager::SendLoginCommand(const char * username, const char * password)
{
    auto promise = std::make_shared<std::promise<bool>>();

    auto task = [promise, username, password, this]() {
        if (!this->client) {
            // No valid client? Set to false immediately
            promise->set_value(false);
            return;
        }

        bool result = this->client->Login(username, password);

        if (result) {
            this->username = username;
        }

        promise->set_value(result);
    };

    std::scoped_lock<std::mutex>(this->clientMutex);

    this->taskQueue.emplace(task);

    this->taskQueueWakeup.notify_all();

    return promise->get_future();
}

std::future<bool> WebClientManager::SendLogoutCommand()
{
    auto promise = std::make_shared<std::promise<bool>>();

    auto task = [promise, this]() {
        if (!this->client) {
            // No valid client? Set to false immediately
            promise->set_value(false);
            return;
        }

        this->client->LogoutAndReset();
        account = WebAccounts::AccountState(); // should effectively reset it

        // We should be logged out
        promise->set_value(!this->client->IsLoggedIn());
    };

    std::scoped_lock<std::mutex>(this->clientMutex);

    this->taskQueue.emplace(task);

    this->taskQueueWakeup.notify_all();

    return promise->get_future();
}

std::future<WebAccounts::AccountState> WebClientManager::SendFetchAccountCommand()
{
    auto promise = std::make_shared<std::promise<WebAccounts::AccountState>>();

    auto task = [promise, this]() {
        if (!this->client) {
            // No valid client? Don't send invalid data. Throw.
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Could not get account data. Client object is invalid.")));
            return;
        }

        this->client->FetchAccount();

        // Download these cards too:
        for (auto&& uuid : BuiltInCards::AsList) {
            this->client->FetchCard(uuid);
        }

        this->account = this->client->GetLocalAccount();
        promise->set_value(this->account);
    };

    std::scoped_lock<std::mutex>(this->clientMutex);

    this->taskQueue.emplace(task);

    this->taskQueueWakeup.notify_all();

    return promise->get_future();
}

std::shared_ptr<sf::Texture> WebClientManager::GetIconForCard(const std::string & uuid)
{
    return iconTextureCache[uuid];
}

std::shared_ptr<sf::Texture> WebClientManager::GetImageForCard(const std::string & uuid)
{
    return cardTextureCache[uuid];
}

const Battle::Card WebClientManager::MakeBattleCardFromWebCardData(const WebAccounts::Card & card)
{
    std::string modelId = card.modelId;
    char code = card.code;

    if (card.modelId.empty()) {
        // try to find fill in the data
        auto cardDataIter = account.cards.find(card.id);

        if (cardDataIter != account.cards.end()) {
            modelId = cardDataIter->second->modelId;
            code = cardDataIter->second->code;
        }
    }
    auto cardModelIter = account.cardModels.find(modelId);

    if (cardModelIter != account.cardModels.end()) {
        auto cardModel = cardModelIter->second;

        return Battle::Card(card.id, code, cardModel->damage, GetElementFromStr(cardModel->element),
            cardModel->name, cardModel->description, cardModel->verboseDescription, 0);
    }

    return Battle::Card();
}

const std::string & WebClientManager::GetUserName() const
{
    return this->username;
}

void WebClientManager::ShutdownAllTasks()
{
    shutdownSignal = true;

    std::unique_lock<std::mutex> lock(clientMutex);
    while (taskQueue.size()) {
        taskQueue.pop();
    }
    lock.unlock();

    this->taskQueueWakeup.notify_all();

    if (tasksThread.joinable()) {
        tasksThread.join();
    }

    if (pingThread.joinable()) {
        pingThread.join();
    }
}
