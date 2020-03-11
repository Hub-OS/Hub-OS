#include "bnWebClientMananger.h"

void WebClientManager::PingThreadHandler()
{
    if (!this->client) return;

    do {
        std::this_thread::sleep_for(std::chrono::microseconds(this->GetPingInterval()));
        std::scoped_lock<std::mutex>(this->clientMutex);
        this->isConnected = this->client->IsOK();
    } while (!shutdownSignal);
}

void WebClientManager::QueuedTasksThreadHandler()
{
    if (!this->client) return;

    std::unique_lock<std::mutex> lock(this->clientMutex);

    do {
        //Wait until we have data
        taskQueueWakeup.wait(lock, [this] {
            return (taskQueue.size());
        });

        //after wait, we own the lock
        if (taskQueue.size())
        {
            auto op = std::move(taskQueue.front());
            taskQueue.pop();

            //unlock now that we're done messing with the queue
            lock.unlock();

            op();

            lock.lock();
        }
    } while (!shutdownSignal);
}

void WebClientManager::InitDownloadImageHandler()
{
    auto callback = [](const char*, WebAccounts::byte*&) -> void {
        // TODO: use SFML http to download raw image data
    };

    std::scoped_lock<std::mutex>(this->clientMutex);
    this->client->SetDownloadImageHandler(callback);
}

WebClientManager::WebClientManager() {
    shutdownSignal = false;

    InitDownloadImageHandler();

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
}

const bool WebClientManager::IsLoggedIn()
{
    return this->client ? this->client->IsLoggedIn() : false;
}

std::future<bool> WebClientManager::SendLoginCommand(const char * username, const char * password)
{
    auto promise = std::make_shared<std::promise<bool>>();

    auto task = [promise, username, password, this]() {
        if (!this->client || this->client->IsOK()) {
            // No valid client? Set to false immediately
            promise->set_value(false);
            return;
        }

        bool result = this->client->Login(username, password);
        promise->set_value(result);
    };

    this->taskQueue.push(std::move(task));

    return promise->get_future();
}

std::future<bool> WebClientManager::SendLogoutCommand()
{
    auto promise = std::make_shared<std::promise<bool>>();

    auto task = [promise, this]() {
        if (!this->client || !this->client->IsOK()) {
            // No valid client? Set to false immediately
            promise->set_value(false);
            return;
        }

        this->client->LogoutAndReset();

        // We should be logged out
        promise->set_value(!this->client->IsLoggedIn());
    };

    this->taskQueue.push(std::move(task));

    return promise->get_future();
}

std::future<WebAccounts::AccountState> WebClientManager::SendFetchAccountCommand()
{
    auto promise = std::make_shared<std::promise<WebAccounts::AccountState>>();

    auto task = [promise, this]() {
        if (!this->client || !this->client->IsOK()) {
            // No valid client? Don't send invalid data. Throw.
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Could not get account data. Client object is invalid.")));
            return;
        }

        this->client->FetchAccount();
        promise->set_value(this->client->GetLocalAccount());
    };

    this->taskQueue.push(std::move(task));

    return promise->get_future();
}

void WebClientManager::ShutdownAllTasks()
{
    shutdownSignal = true;

    if (tasksThread.joinable()) {
        tasksThread.join();
    }

    if (pingThread.joinable()) {
        pingThread.join();
    }

    while (taskQueue.size()) {
        taskQueue.pop();
    }
}
