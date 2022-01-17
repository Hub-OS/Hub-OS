#include "bnIdentityManager.h"

#include "bnServerAssetManager.h"
#include "../bnFileUtil.h"
#include <Poco/RandomStream.h>

constexpr std::streamsize IDENTITY_LENGTH = 32;

#define IDENTITY_FOLDER std::filesystem::path("identity")

namespace Overworld {
  IdentityManager::IdentityManager(const std::string& host, uint16_t port)
  {
    path = IDENTITY_FOLDER / URIEncode(host + "_p" + std::to_string(port));
  }

  std::string IdentityManager::GetIdentity() {
    if (!identity.empty()) {
      return identity;
    }

    identity = FileUtil::Read(path);

    if (identity.empty()) {
      // make sure the identity folder exists
      std::filesystem::create_directories(IDENTITY_FOLDER);

      char buffer[IDENTITY_LENGTH];

      Poco::RandomBuf randomBuf;
      int read = randomBuf.readFromDevice(buffer, IDENTITY_LENGTH);

      identity = std::string(buffer, read);

      auto writeStream = FileUtil::WriteStream(path);
      writeStream << identity;
    }

    return identity;
  }
}
