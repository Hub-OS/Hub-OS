#include "bnPackageAddress.h"

#include <string.h>
#include <tuple>

PackageAddress::PackageAddress() {}

PackageAddress::PackageAddress(const std::string& ns, const std::string& id) {
    namespaceId = ns;
    packageId = id;
}

PackageAddress::PackageAddress(const PackageAddress& rhs) {
    namespaceId = rhs.namespaceId;
    packageId = rhs.packageId;
}

PackageAddress::operator std::string() const {
    if (namespaceId.empty()) {
        return packageId;
    }

    return namespaceId + nsDelim + packageId;
}

bool operator<(const PackageAddress& a, const PackageAddress& b) {
    std::string a_str = a;
    std::string b_str = b;
    return strcmp(a_str.c_str(), b_str.c_str()) < 0;
}

bool operator==(const PackageAddress& a, const PackageAddress& b)
{
  std::string a_str = a;
  std::string b_str = b;
  return strcmp(a_str.c_str(), b_str.c_str()) == 0;
}

stx::result_t<PackageAddress> PackageAddress::FromStr(const std::string& fqn) {
  std::vector<std::string> tokens = stx::tokenize(fqn, nsDelim);

  if (tokens.empty()) return stx::error<PackageAddress>("Tokenization was empty for `" + fqn + "`");
  if (tokens.size() == 1) return stx::ok(PackageAddress{ "", tokens[0] });

  return stx::ok(PackageAddress{ tokens[0], tokens[1] });
}

// PackageHash struct operators for std containers
bool operator<(const PackageHash& a, const PackageHash& b) {
  return std::tie(a.packageId, a.md5) < std::tie(b.packageId, b.md5);
}

bool operator==(const PackageHash& a, const PackageHash& b)
{
  return std::tie(a.packageId, a.md5) == std::tie(b.packageId, b.md5);
}
