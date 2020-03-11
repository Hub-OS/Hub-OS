#pragma once
#include <WebAPI/client/WebClient.h>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <future>

template<typename T>
bool is_ready(const std::future<T>& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

class WebClientManager {
private:
    std::unique_ptr<WebAccounts::WebClient> client;
    std::thread tasksThread; //!< This thread performs api requests when queued by the battle engine
    std::thread pingThread; //!< This thread pings the server to gauge the connection
    std::mutex clientMutex; //!< Mutex for WebClient since it may be accessed by multiple threads...
    char isConnected; //!< True if pinged heartbeat endpoint successfully in the last invernal
    long heartbeatInterval; //!< The time in milliseconds to ping the web server
    std::queue<std::function<void()>> taskQueue;
    std::condition_variable taskQueueWakeup;
    bool shutdownSignal;

    void PingThreadHandler();
    void QueuedTasksThreadHandler();
    void InitDownloadImageHandler();
public:
    WebClientManager();
    static WebClientManager& GetInstance();
    void PingInterval(long interval);
    const long GetPingInterval() const;
    void ConnectToWebServer(const char* apiVersion, const char* domain, int port);
    const bool IsConnectedToWebServer();
    const bool IsLoggedIn();

    std::future<bool> SendLoginCommand(const char* username, const char* password);
    std::future<bool> SendLogoutCommand();
    std::future<WebAccounts::AccountState> SendFetchAccountCommand();

    void ShutdownAllTasks();
};

/*! \brief Shorthand to get instance of the manager */
#define WEBCLIENT WebClientManager::GetInstance()