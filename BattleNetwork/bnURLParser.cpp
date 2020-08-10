#include "bnURLParser.h"

#include <string>
#include <algorithm>
#include <cctype>
#include <functional>
using namespace std;

URL::URL(const string& url_s) : host(), path(), query(), protocol(), port() {
  std::string url = url_s;

  // assume http:// if no protocol included
  if (url.find("://") == std::string::npos) {
    url = "http://" + url;
  }
  
  parse(url);
}

const std::string& URL::GetProtocol() const
{
    return protocol;
}

const std::string& URL::GetHost() const
{
    return host;
}

const std::string& URL::GetPath() const
{
    return path;
}

const std::string& URL::GetQuery() const
{
    return query;
}

const std::string& URL::GetPort() const
{
  return port;
}

void URL::parse(const string& url_s)
{
    const string prot_end("://");
    string::const_iterator prot_i = search(url_s.begin(), url_s.end(), prot_end.begin(), prot_end.end());
    protocol.reserve(distance(url_s.begin(), prot_i));
    transform(url_s.begin(), prot_i, back_inserter(protocol),tolower); // protocol is icase
    if (prot_i == url_s.end())
        return;
    advance(prot_i, prot_end.length());
    string::const_iterator path_i = find(prot_i, url_s.end(), '/');
    host.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i, back_inserter(host), tolower); // host is icase
    string::const_iterator query_i = find(path_i, url_s.end(), '?');
    path.assign(path_i, query_i);
    if (query_i != url_s.end())
        ++query_i;
    query.assign(query_i, url_s.end());

    // subdivide host string into domain + port
    const string port_s(":");
    string::const_iterator port_i = search(host.cbegin(), host.cend(), port_s.begin(), port_s.end());

    if (port_i == host.cend()) {
      port = "80";
      return;
    }

    ++port_i; // skip the colon

    ptrdiff_t realHostLen = distance(host.cbegin(), port_i);
    port.reserve(host.length() - realHostLen);
    port.assign(port_i, host.cend());
    host = host.substr(0, realHostLen-1); // skip the colon ':'
}