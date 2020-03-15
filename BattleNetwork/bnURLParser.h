#pragma once
  
#include <string>

struct URL {
    URL(const std::string& url_s);
    const std::string& GetProtocol() const;
    const std::string& GetHost() const;
    const std::string& GetPath() const;
    const std::string& GetQuery() const;
private:
    void parse(const std::string& url_s);
private:
    std::string protocol, host, path, query;
};