#pragma once
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>

struct CurrentTime {
    static std::string Get() {
        auto time = std::time(nullptr);
        auto timestamp = std::put_time(std::localtime(&time), "%y-%m-%d %OH:%OM:%OS");

        std::stringstream ss;
        ss << timestamp;
        return  ss.str();
    }
};