#include "bnGameSession.h"
#include "netplay/bnBufferWriter.h"
#include "netplay/bnBufferReader.h"

const bool GameSession::LoadSession(const std::string& inpath)
{
  monies = 0;
  cardPool.clear();
  keys.clear();
  keyItems.clear();

  BufferReader reader;
  std::fstream file;
  file.open(inpath, std::ios::out | std::ios::binary);

  if (!file.is_open())
    return false;

  size_t file_begin = file.tellg();
  file.seekg(0, std::ios::end);
  size_t file_end = file.tellg();
  size_t file_len = file_end - file_begin;
  Poco::Buffer<char> buffer(file_len);

  std::string nick = reader.ReadTerminatedString(buffer);

  size_t keys_len = reader.Read<size_t>(buffer);

  while (keys_len-- > 0) {
    std::string key = reader.ReadTerminatedString(buffer);
    std::string val = reader.ReadTerminatedString(buffer);
    SetKey(key, val);
  }

  monies = reader.Read<decltype(monies)>(buffer);

  size_t pool_len = reader.Read<size_t>(buffer);

  while (pool_len-- > 0) {
    std::string card = reader.ReadTerminatedString(buffer);
    cardPool.push_back(card);
  }

  file.close();

  return true;
}

void GameSession::SaveSession(const std::string& outpath)
{
  size_t file_len = nickname.size() + (keys.size() * 2u) + sizeof(monies) + cardPool.size();

  BufferWriter writer;
  Poco::Buffer<char> buffer(file_len);
  std::fstream file;
  file.open(outpath, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return;

  writer.WriteTerminatedString(buffer, nickname);

  size_t keys_len = keys.size();
  writer.Write(buffer, keys_len);

  for (auto& [k, v] : keys) {
    writer.WriteTerminatedString(buffer, k);
    writer.WriteTerminatedString(buffer, v);
  }

  size_t pool_len = cardPool.size();
  writer.Write(buffer, pool_len);

  for (auto& card : cardPool) {
    writer.WriteTerminatedString(buffer, card);
  }
}

void GameSession::SetKey(const std::string& key, const std::string& value)
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

const std::string GameSession::GetValue(const std::string& key)
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