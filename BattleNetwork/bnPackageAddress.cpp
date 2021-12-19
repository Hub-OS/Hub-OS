#include "bnPackageAddress.h"

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