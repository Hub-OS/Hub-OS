#pragma once
#include "stx/result.h"
#include "stx/string.h"

struct PackageAddress {
  static const char nsDelim = '/';

  std::string namespaceId, packageId;

  PackageAddress();
  PackageAddress(const std::string& ns, const std::string& id);
  PackageAddress(const PackageAddress& rhs);
  operator std::string() const;

  static stx::result_t<PackageAddress> FromStr(const std::string& fqn);
};

bool operator<(const PackageAddress& a, const PackageAddress& b);

stx::result_t<PackageAddress> PackageAddress::FromStr(const std::string& fqn) {
    std::vector<std::string> tokens = stx::tokenize(fqn, nsDelim);

    if (tokens.empty()) return stx::error<PackageAddress>("Tokenization was empty for `" + fqn + "`");
    if (tokens.size() == 1) return stx::ok(PackageAddress{ "", tokens[0] });

    return stx::ok(PackageAddress{ tokens[0], tokens[1] });
}