#pragma once
#include <string>
#include <vector>
#include <map>
#include "bnCardFolderCollection.h"
#include "bnPackageAddress.h"

class CardPackageManager;
class BlockPackageManager;

class GameSession {
  using KeyItem = std::pair<std::string, std::string>;
  CardFolderCollection folders;
  std::vector<std::string> cardPool;
  std::vector<KeyItem> keyItems;
  std::vector<PackageHash> whitelist; //!< Used to enforce server restrictions in select scenes
  std::map<std::string, std::string> keys; // key-value table
  uint32_t monies{}; //!< Monies on the account
  std::string nickname = "Anon";

  CardPackageManager* cardPackages{ nullptr };
  BlockPackageManager* blockPackages{ nullptr };
public:
  const bool LoadSession(const std::filesystem::path& inpath);
  const bool IsPackageAllowed(const PackageHash& hash) const; // utility function to compare package with whitelist
  const bool IsFolderAllowed(CardFolder* folder) const;
  const bool IsBlockAllowed(const std::string& packageId) const;
  void SaveSession(const std::filesystem::path& outpath);
  void SetKeyValue(const std::string& key, const std::string& value);
  void SetNick(const std::string& nickname);
  void SetCardPackageManager(CardPackageManager& cardPackageManager);
  void SetBlockPackageManager(BlockPackageManager& blockPackageManager);
  void SetWhitelist(const std::vector<PackageHash> whitelist);
  const std::string GetKeyValue(const std::string& key) const;
  const std::string GetNick() const;
  CardFolderCollection& GetCardFolderCollection();
};
