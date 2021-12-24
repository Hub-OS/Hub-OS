#pragma once
#include <string>
#include <vector>
#include <map>
#include "bnCardFolderCollection.h"
#include "bnPackageAddress.h"

class CardPackageManager;

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

  // utility function to compare package with whitelist
  const bool IsPackageAllowed(const PackageHash& hash) const;

public:
  const bool LoadSession(const std::string& inpath);
  const bool IsFolderAllowed(CardFolder* folder) const;
  void SaveSession(const std::string& outpath);
  void SetKeyValue(const std::string& key, const std::string& value);
  void SetNick(const std::string& nickname);
  void SetCardPackageManager(CardPackageManager& cardPackageManager);
  void SetWhitelist(const std::vector<PackageHash> whitelist);
  const std::string GetKeyValue(const std::string& key) const;
  const std::string GetNick() const;
  CardFolderCollection& GetCardFolderCollection();
};