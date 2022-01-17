#include "bnGameSession.h"
#include "bnCardPackageManager.h"
#include "bnBlockPackageManager.h"
#include "netplay/bnBufferWriter.h"
#include "netplay/bnBufferReader.h"

const bool GameSession::LoadSession(const std::filesystem::path& inpath)
{
  monies = 0;
  cardPool.clear();
  keys.clear();
  keyItems.clear();

  BufferReader reader;
  std::fstream file;
  file.open(inpath, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return false;

  size_t file_begin = file.tellg();
  file.seekg(0, std::ios::end);
  size_t file_end = file.tellg();
  size_t file_len = file_end - file_begin;
  Poco::Buffer<char> buffer(file_len);
  file.seekg(0, std::ios::beg);
  file.read(buffer.begin(), file_len);

  nickname = reader.ReadTerminatedString(buffer);

  size_t keys_len = reader.Read<size_t>(buffer);

  while (keys_len-- > 0) {
    std::string key = reader.ReadString<uint64_t>(buffer);
    std::string val = reader.ReadString<uint64_t>(buffer);
    SetKeyValue(key, val);
  }

  size_t pool_len = reader.Read<size_t>(buffer);

  while (pool_len-- > 0) {
    std::string card = reader.ReadTerminatedString(buffer);
    cardPool.push_back(card);
  }

  // create new collection
  folders.ClearCollection();

  if (!cardPackages) {
    Logger::Log(LogLevel::critical, "In GameSession::LoadSession() cardPackages was nullptr! No card folders will be loaded.");
    return false;
  }

  // get the folder collection size
  size_t collection_len = reader.Read<size_t>(buffer);

  for (size_t i = 0; i < collection_len; i++) {
    CardFolder* folder;

    // read folder name
    std::string folder_name = reader.ReadString<char>(buffer);

    if (!folders.MakeFolder(folder_name)) {
      Logger::Logf(LogLevel::critical, "Unable to create folder `%s`", folder_name.c_str());
      continue;
    }

    folders.GetFolder(folder_name, folder);

    // save folder size
    size_t folder_len = reader.Read<size_t>(buffer);

    for (size_t j = 0; j < folder_len; j++) {
      // read name
      std::string id = reader.ReadString<char>(buffer);

      // read code
      char code = reader.Read<char>(buffer);

      // read name
      std::string name = reader.ReadString<char>(buffer);

      Battle::Card::Properties props;

      if (cardPackages->HasPackage(id)) {
        CardMeta& meta = cardPackages->FindPackageByID(id);
        props = meta.GetCardProperties();
        props.code = code;

        if (std::find(meta.codes.begin(), meta.codes.end(), code) == meta.codes.end()) {
          props.code = *meta.codes.begin();
        }
      }
      else {
        Logger::Logf(LogLevel::critical, "Unable to create battle card from mod package %s", id.c_str());
        props.description = id;
        props.uuid = id;
        props.shortname = name;
        props.code = code;
      }

      folder->AddCard(props);
    }
  }

  file.close();

  return true;
}

const bool GameSession::IsPackageAllowed(const PackageHash& hash) const
{
  if (whitelist.empty()) return true;
  return std::find(whitelist.begin(), whitelist.end(), hash) != whitelist.end();
}

const bool GameSession::IsFolderAllowed(CardFolder* folder) const
{
  // as long as ALL contents in the folder match, the folder is allowed
  for (CardFolder::Iter iter = folder->Begin(); iter != folder->End(); iter++) {
    PackageHash hash;
    hash.packageId = (*iter)->GetUUID();

    if (!cardPackages->HasPackage(hash.packageId)) continue;

    hash.md5 = cardPackages->FindPackageByID(hash.packageId).GetPackageFingerprint();

    if (IsPackageAllowed(hash)) continue;

    return false;
  }

  return true;
}

const bool GameSession::IsBlockAllowed(const std::string& packageId) const
{
  if (!blockPackages->HasPackage(packageId)) return false;

  const std::string& md5 = blockPackages->FindPackageByID(packageId).GetPackageFingerprint();
  return IsPackageAllowed(PackageHash{ packageId, md5 });
}

void GameSession::SaveSession(const std::filesystem::path& outpath)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer(0);
  std::fstream file;
  file.open(outpath, std::ios::out | std::ios::binary);

  if (!file.is_open())
    return;

  // save nickname
  writer.WriteTerminatedString(buffer, nickname);

  // save key-values
  size_t num_of_keys = keys.size();
  writer.Write(buffer, num_of_keys);

  for (auto& [k, v] : keys) {
    writer.WriteString<uint64_t>(buffer, k);
    writer.WriteString<uint64_t>(buffer, v);
  }

  // save card pool
  size_t pool_len = cardPool.size();
  writer.Write(buffer, pool_len);

  for (auto& card : cardPool) {
    writer.WriteTerminatedString(buffer, card);
  }

  // save card folder collection size
  std::vector<std::string> folder_names = folders.GetFolderNames();
  size_t collection_len = folder_names.size();
  writer.Write(buffer, collection_len);

  for (const std::string& name : folder_names) {
    CardFolder* folder;
    if (folders.GetFolder(name, folder)) {
      // save folder name
      writer.WriteString<char>(buffer, name);

      // save folder size
      size_t folder_len = folder->GetSize();
      writer.Write(buffer, folder_len);

      // save folder contents
      for (auto iter = folder->Begin(); iter != folder->End(); iter++) {
        // save card id
        std::string id = (*iter)->GetUUID();

        // save card code
        char code = (*iter)->GetCode();

        // save card name
        std::string name = (*iter)->GetShortName();

        writer.WriteString<char>(buffer, id.c_str());
        writer.WriteBytes<char>(buffer, &code, sizeof(char));
        writer.WriteString<char>(buffer, name.c_str());
      }
    }
  }

  // copy to a std string so we can write-out to the file
  char* raw = new char [buffer.size()];
  std::memcpy(raw, buffer.begin(), buffer.size());
  std::string as_string = std::string(raw, buffer.size());
  file << as_string;
  file.close();
  delete[] raw;
}

void GameSession::SetKeyValue(const std::string& key, const std::string& value)
{
  auto iter = keys.find(key);

  if (iter != keys.end()) {
    keys[key] = value;
  }
  else {
    keys.insert(std::make_pair(key, value));
  }
}

void GameSession::SetNick(const std::string& nickname) {
  this->nickname = nickname;
}

void GameSession::SetCardPackageManager(CardPackageManager& cardPackageManager)
{
  cardPackages = &cardPackageManager;
}

void GameSession::SetBlockPackageManager(BlockPackageManager& blockPackageManager)
{
  blockPackages = &blockPackageManager;
}

void GameSession::SetWhitelist(const std::vector<PackageHash> whitelist)
{
  this->whitelist = whitelist;
}

const std::string GameSession::GetKeyValue(const std::string& key) const
{
  std::string res;

  auto iter = keys.find(key);
  if (iter != keys.end()) {
    res = iter->second;
  }

  return res;
}

const std::string GameSession::GetNick() const
{
  return nickname;
}

CardFolderCollection& GameSession::GetCardFolderCollection()
{
  return folders;
}
