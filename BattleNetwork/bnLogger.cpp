#include "bnLogger.h"

// explicitely define static member variables
std::mutex Logger::m;
std::queue<std::string> Logger::logs = std::queue<std::string>();
std::ofstream Logger::file;
uint8_t Logger::logLevel = LogLevel::critical;