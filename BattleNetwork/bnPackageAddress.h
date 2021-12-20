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
  const bool HasID() const { return !packageId.empty(); }

  static stx::result_t<PackageAddress> FromStr(const std::string& fqn);
};

bool operator<(const PackageAddress& a, const PackageAddress& b);
bool operator==(const PackageAddress& a, const PackageAddress& b);